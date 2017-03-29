#ifndef DECAF_NOGL

#include "clilog.h"
#include <common/decaf_assert.h>
#include "config.h"
#include "decafsdl_opengl.h"
#include <string>
#include <glbinding/Binding.h>
#include <glbinding/Meta.h>
#include "vr.h"
#include "vrogl.h"

TextureBuffer vrWorldEye[2], vrTV, vrDRC;

static std::string
getGlDebugSource(gl::GLenum source)
{
   switch (source) {
   case gl::GL_DEBUG_SOURCE_API:
      return "API";
   case gl::GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      return "WINSYS";
   case gl::GL_DEBUG_SOURCE_SHADER_COMPILER:
      return "COMPILER";
   case gl::GL_DEBUG_SOURCE_THIRD_PARTY:
      return "EXTERNAL";
   case gl::GL_DEBUG_SOURCE_APPLICATION:
      return "APP";
   case gl::GL_DEBUG_SOURCE_OTHER:
      return "OTHER";
   default:
      return glbinding::Meta::getString(source);
   }
}

static std::string
getGlDebugType(gl::GLenum severity)
{
   switch (severity) {
   case gl::GL_DEBUG_TYPE_ERROR:
      return "ERROR";
   case gl::GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      return "DEPRECATED_BEHAVIOR";
   case gl::GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      return "UNDEFINED_BEHAVIOR";
   case gl::GL_DEBUG_TYPE_PORTABILITY:
      return "PORTABILITY";
   case gl::GL_DEBUG_TYPE_PERFORMANCE:
      return "PERFORMANCE";
   case gl::GL_DEBUG_TYPE_MARKER:
      return "MARKER";
   case gl::GL_DEBUG_TYPE_PUSH_GROUP:
      return "PUSH_GROUP";
   case gl::GL_DEBUG_TYPE_POP_GROUP:
      return "POP_GROUP";
   case gl::GL_DEBUG_TYPE_OTHER:
      return "OTHER";
   default:
      return glbinding::Meta::getString(severity);
   }
}

static std::string
getGlDebugSeverity(gl::GLenum severity)
{
   switch (severity) {
   case gl::GL_DEBUG_SEVERITY_HIGH:
      return "HIGH";
   case gl::GL_DEBUG_SEVERITY_MEDIUM:
      return "MED";
   case gl::GL_DEBUG_SEVERITY_LOW:
      return "LOW";
   case gl::GL_DEBUG_SEVERITY_NOTIFICATION:
      return "NOTIF";
   default:
      return glbinding::Meta::getString(severity);
   }
}

static void GL_APIENTRY
debugMessageCallback(gl::GLenum source, gl::GLenum type, gl::GLuint id, gl::GLenum severity,
   gl::GLsizei length, const gl::GLchar* message, const void *userParam)
{
   for (auto filterID : decaf::config::gpu::debug_filters) {
      if (filterID == id) {
         return;
      }
   }

   auto outputStr = fmt::format("GL Message ({}, {}, {}, {}) {}", id,
      getGlDebugSource(source),
      getGlDebugType(type),
      getGlDebugSeverity(severity),
      message);

   if (severity == gl::GL_DEBUG_SEVERITY_HIGH) {
      gCliLog->warn(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_MEDIUM) {
      gCliLog->debug(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_LOW) {
      gCliLog->trace(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_NOTIFICATION) {
      gCliLog->info(outputStr);
   } else {
      gCliLog->info(outputStr);
   }
}

DecafSDLOpenGL::DecafSDLOpenGL()
{
    using decaf::config::ui::background_colour;
    
    mBackgroundColour[0] = background_colour.r / 255.0f;
    mBackgroundColour[1] = background_colour.g / 255.0f;
    mBackgroundColour[2] = background_colour.b / 255.0f;
}

DecafSDLOpenGL::~DecafSDLOpenGL()
{
   if (mContext) {
      SDL_GL_DeleteContext(mContext);
      mContext = nullptr;
   }

   if (mThreadContext) {
      SDL_GL_DeleteContext(mThreadContext);
      mThreadContext = nullptr;
   }
}

void
DecafSDLOpenGL::InitOculusTextures()
{
  for (int eye = 0; eye < 2; eye++)
  {
    renderTarget = ovr_GetFovTextureSize(hmdSession, (ovrEyeType)eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
    vrWorldEye[eye] = TextureBuffer(hmdSession, true, true, renderTarget.w, renderTarget.h, 1, NULL);
    vrWorldEye[eye].Commit();
  }
  vrTV = TextureBuffer(hmdSession, true, true, renderTarget.w, renderTarget.h, 1, NULL);
  vrDRC = TextureBuffer(hmdSession, true, true, renderTarget.w, renderTarget.h, 1, NULL);
  vrTV.Commit();
  vrDRC.Commit();
}

void
DecafSDLOpenGL::initialiseContext()
{
   glbinding::Binding::initialize();
   glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After | glbinding::CallbackMask::ParametersAndReturnValue, { "glGetError" });
   glbinding::setAfterCallback([](const glbinding::FunctionCall &call) {
      auto error = glbinding::Binding::GetError.directCall();

      if (error != gl::GL_NO_ERROR) {
         fmt::MemoryWriter writer;
         writer << call.function->name() << "(";

         for (unsigned i = 0; i < call.parameters.size(); ++i) {
            writer << call.parameters[i]->asString();
            if (i < call.parameters.size() - 1)
               writer << ", ";
         }

         writer << ")";

         if (call.returnValue) {
            writer << " -> " << call.returnValue->asString();
         }

         gCliLog->error("OpenGL error: {} with {}", glbinding::Meta::getString(error), writer.str());
      }
   });

   if (decaf::config::gpu::debug) {
      gl::glDebugMessageCallback(&debugMessageCallback, nullptr);
      gl::glEnable(gl::GL_DEBUG_OUTPUT);
      gl::glEnable(gl::GL_DEBUG_OUTPUT_SYNCHRONOUS);
   }
}

void
DecafSDLOpenGL::initialiseDraw()
{
   static auto vertexCode = R"(
      #version 420 core
      in vec2 fs_position;
      in vec2 fs_texCoord;
      out vec2 vs_texCoord;

      out gl_PerVertex {
         vec4 gl_Position;
      };

      void main()
      {
         vs_texCoord = fs_texCoord;
         gl_Position = vec4(fs_position, 0.0, 1.0);
      })";

   static auto pixelCode = R"(
      #version 420 core
      in vec2 vs_texCoord;
      out vec4 ps_color;
      uniform sampler2D sampler_0;

      void main()
      {
         ps_color = texture(sampler_0, vs_texCoord);
      })";

   // Create vertex program
   mVertexProgram = gl::glCreateShaderProgramv(gl::GL_VERTEX_SHADER, 1, &vertexCode);

   // Create pixel program
   mPixelProgram = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, &pixelCode);
   gl::glBindFragDataLocation(mPixelProgram, 0, "ps_color");

   // Create pipeline
   gl::glGenProgramPipelines(1, &mPipeline);
   gl::glUseProgramStages(mPipeline, gl::GL_VERTEX_SHADER_BIT, mVertexProgram);
   gl::glUseProgramStages(mPipeline, gl::GL_FRAGMENT_SHADER_BIT, mPixelProgram);

   // (TL, TR, BR)    (BR, BL, TL)
   // Create vertex buffer
   static const gl::GLfloat vertices[] = {
      -1.0f,  -1.0f,   0.0f, 1.0f,
      1.0f,  -1.0f,   1.0f, 1.0f,
      1.0f, 1.0f,   1.0f, 0.0f,

      1.0f, 1.0f,   1.0f, 0.0f,
      -1.0f, 1.0f,   0.0f, 0.0f,
      -1.0f,  -1.0f,   0.0f, 1.0f,
   };

   gl::glCreateBuffers(1, &mVertBuffer);
   gl::glNamedBufferData(mVertBuffer, sizeof(vertices), vertices, gl::GL_STATIC_DRAW);

   // Create vertex array
   gl::glCreateVertexArrays(1, &mVertArray);

   auto fs_position = gl::glGetAttribLocation(mVertexProgram, "fs_position");
   gl::glEnableVertexArrayAttrib(mVertArray, fs_position);
   gl::glVertexArrayAttribFormat(mVertArray, fs_position, 2, gl::GL_FLOAT, gl::GL_FALSE, 0);
   gl::glVertexArrayAttribBinding(mVertArray, fs_position, 0);

   auto fs_texCoord = gl::glGetAttribLocation(mVertexProgram, "fs_texCoord");
   gl::glEnableVertexArrayAttrib(mVertArray, fs_texCoord);
   gl::glVertexArrayAttribFormat(mVertArray, fs_texCoord, 2, gl::GL_FLOAT, gl::GL_FALSE, 2 * sizeof(gl::GLfloat));
   gl::glVertexArrayAttribBinding(mVertArray, fs_texCoord, 0);

   // Create texture sampler
   gl::glGenSamplers(1, &mSampler);

   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_LINEAR));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_LINEAR));
}

void
DecafSDLOpenGL::drawScanBuffer(gl::GLuint object)
{
   // Setup screen draw shader
   gl::glBindVertexArray(mVertArray);
   gl::glBindVertexBuffer(0, mVertBuffer, 0, 4 * sizeof(gl::GLfloat));
   gl::glBindProgramPipeline(mPipeline);

   // Draw screen quad
   gl::glBindSampler(0, mSampler);
   gl::glBindTextureUnit(0, object);

   gl::glDrawArrays(gl::GL_TRIANGLES, 0, 6);
}

void
DecafSDLOpenGL::drawScanBuffers(Viewport &tvViewport,
                                gl::GLuint tvBuffer,
                                Viewport &drcViewport,
                                gl::GLuint drcBuffer)
{
   // Set up some needed GL state
   gl::glColorMaski(0, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
   gl::glDisablei(gl::GL_BLEND, 0);
   gl::glDisable(gl::GL_DEPTH_TEST);
   gl::glDisable(gl::GL_STENCIL_TEST);
   gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glDisable(gl::GL_CULL_FACE);

   // Clear screen
   gl::glClearColor(mBackgroundColour[0], mBackgroundColour[1], mBackgroundColour[2], 1.0f);
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);

   // Draw displays
   // DRC screen width = 13.683735932855423761961780246585 cm
   // DRC screen height = 7.6971014622311758661035013887042 cm
   auto drawTV = tvViewport.width > 0 && tvViewport.height > 0;
   auto drawDRC = drcViewport.width > 0 && drcViewport.height > 0;

   //gl::wglSwapIntervalEXT(0);

   ovrSessionStatus ss;
   ovr_GetSessionStatus(hmdSession, &ss);
   if (ss.ShouldRecenter)
     ovr_RecenterTrackingOrigin(hmdSession);
   if (ss.IsVisible)
   {
     ovrInputState t;
     ovr_GetInputState(hmdSession, ovrControllerType_Touch, &t);

     vrTV.SetAndClearRenderSurface();
     drawScanBuffer(tvBuffer);
     vrTV.Commit();

     ovrTrackingState s = ovr_GetTrackingState(hmdSession, ovr_GetPredictedDisplayTime(hmdSession, 0), true);

     vrDRC.SetAndClearRenderSurface();
     drawScanBuffer(drcBuffer);
     vrDRC.Commit();

     ovrLayerQuad ltv, ltv2, ldrc, lfinger, ltouch;
     ltv.Header.Type = ovrLayerType_Quad;
     ltv.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
     ltv.ColorTexture = vrTV.TextureChain;
     ltv.Viewport = vrTV.viewport;
     ltv.QuadSize.x = 3.0f; // metres
     ltv.QuadSize.y = ltv.QuadSize.x * 9.0f / 16.0f; // metres
     ltv.QuadPoseCenter.Position.x = 0; // metres
     ltv.QuadPoseCenter.Position.y = 0; // metres
     ltv.QuadPoseCenter.Position.z = -1.5f; // metres (negative means in front of us)
     ltv.QuadPoseCenter.Orientation.w = 1;
     ltv.QuadPoseCenter.Orientation.x = 0;
     ltv.QuadPoseCenter.Orientation.y = 0;
     ltv.QuadPoseCenter.Orientation.z = 0;

     // draw another one behind us
     ltv2 = ltv;
     ltv2.QuadPoseCenter.Orientation.w = 0;
     ltv2.QuadPoseCenter.Orientation.y = 1;
     ltv2.QuadPoseCenter.Position.z = -ltv.QuadPoseCenter.Position.z;

     // Gamepad layer is attatched to left hand
     ldrc.Header.Type = ovrLayerType_Quad;
     ldrc.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
     ldrc.ColorTexture = vrDRC.TextureChain;
     ldrc.Viewport = vrDRC.viewport;
     ldrc.QuadSize.x = 0.136837359f; // metres
     ldrc.QuadSize.y = ldrc.QuadSize.x * 9.0f / 16.0f; // metres
     OVR::Posef p;
     if (s.HandStatusFlags[0] & ovrStatus_PositionTracked | ovrStatus_OrientationTracked)
     {
       p = s.HandPoses[0].ThePose;
       OVR::Vector3f o = { ldrc.QuadSize.x / 2 + 0.035916f, 0, 0.055f };
       p.Translation += p.Rotate(o);
     }
     else
     {
       p.Translation.x = 0; // metres
       p.Translation.y = 0; // metres
       p.Translation.z = -0.4f; // metres (negative means in front of us)
       p.Rotation.w = 1;
       p.Rotation.x = 0;
       p.Rotation.y = 0;
       p.Rotation.z = 0;
     }
     OVR::Vector3f v = { -90 * MATH_FLOAT_DEGREETORADFACTOR, 0, 0 };
     OVR::Quatf r;
     r = OVR::Quatf::FromRotationVector(v);
     p.Rotation *= r;
     ldrc.QuadPoseCenter = p;

     // finger is attatched to right hand
     if (t.Touches & ovrTouch_RIndexPointing)
       lfinger.Header.Type = ovrLayerType_Quad;
     else
       lfinger.Header.Type = ovrLayerType_Disabled;
     lfinger.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
     lfinger.ColorTexture = vrDRC.TextureChain;
     lfinger.Viewport = vrDRC.viewport;
     lfinger.Viewport.Size.w = 16; // use left 16 pixels of screen as finger image
     lfinger.QuadSize.x = 0.01f; // metres
     lfinger.QuadSize.y = 0.09f; // metres
     if (s.HandStatusFlags[1] & ovrStatus_PositionTracked | ovrStatus_OrientationTracked)
     {
       p = s.HandPoses[1].ThePose;
       OVR::Vector3f o = { 0, -0.01f, -0.02f };
       p.Translation += p.Rotate(o);
     }
     else
     {
       lfinger.Header.Type = ovrLayerType_Disabled;
     }
     v = { -100 * MATH_FLOAT_DEGREETORADFACTOR, 0, 0 };
     r = OVR::Quatf::FromRotationVector(v);
     p.Rotation *= r;
     lfinger.QuadPoseCenter = p;
     OVR::Vector3f tip = { 0, lfinger.QuadSize.y / 2, 0 };
     tip = p.Translation + p.Rotate(tip);
     r = ldrc.QuadPoseCenter.Orientation;
     OVR::Vector3f touchpoint = r.InverseRotate(tip - ldrc.QuadPoseCenter.Position);
     vrTouchPoint.x = (touchpoint.x / ldrc.QuadSize.x) + 0.5f;
     vrTouchPoint.y = 0.5f - (touchpoint.y / ldrc.QuadSize.y);
     vrTouchPoint.z = touchpoint.z;
     vrTouching = (touchpoint.z < 0 && touchpoint.z > -0.05f) && (vrTouchPoint.x > -0.1f && vrTouchPoint.x < 1.1f) && (vrTouchPoint.y > -0.1f && vrTouchPoint.y < 1.1f);

#if 0
     ltouch.Header.Type = ovrLayerType_Quad;
     ltouch.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
     ltouch.ColorTexture = vrDRC.TextureChain;
     ltouch.Viewport = vrDRC.viewport;
     ltouch.QuadSize.x = 0.02f; // metres
     ltouch.QuadSize.y = 0.02f; // metres
     v = { -90 * MATH_FLOAT_DEGREETORADFACTOR, 0, 0 };
     r = OVR::Quatf::FromRotationVector(v);
     if (s.HandStatusFlags[1] & ovrStatus_PositionTracked | ovrStatus_OrientationTracked)
     {
       p = s.HandPoses[0].ThePose;
       OVR::Vector3f o = { ldrc.QuadSize.x / 2 + 0.035916f, 0, 0.055f };
       p.Translation += p.Rotate(o);
     }
     else
     {
       ltouch.Header.Type = ovrLayerType_Disabled;
     }
     p.Rotation *= r;
     r = p.Rotation;
     p.Translation += r.Rotate(touchpoint);
     ltouch.QuadPoseCenter = p;
     if (!vrTouching)
       ltouch.Header.Type = ovrLayerType_Disabled;
#endif

     ovrLayerHeader* LayerList[5];
     LayerList[0] = &ltv.Header;
     LayerList[1] = &ltv2.Header;
     LayerList[2] = &ldrc.Header;
     LayerList[3] = &lfinger.Header;
     //LayerList[4] = &ltouch.Header;
     ovrResult result = ovr_SubmitFrame(hmdSession, 0, NULL, LayerList, 4);
     if (result == ovrSuccess_NotVisible)
     {
     }
     else if (result == ovrError_DisplayLost)
     {
     }
     else if (OVR_FAILURE(result))
     {
     }
   }

   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
   if (drawTV) {
      float viewportArray[] = {
         tvViewport.x, tvViewport.y,
         tvViewport.width, tvViewport.height
      };

      gl::glViewportArrayv(0, 1, viewportArray);
      drawScanBuffer(tvBuffer);
   }

   if (drawDRC) {
      float viewportArray[] = {
         drcViewport.x, drcViewport.y,
         drcViewport.width, drcViewport.height
      };

      gl::glViewportArrayv(0, 1, viewportArray);
      drawScanBuffer(drcBuffer);
   }

   // Draw UI
   int width, height;
   SDL_GetWindowSize(mWindow, &width, &height);
   decaf::debugger::drawUiGL(width, height);

   // Swap
   SDL_GL_SwapWindow(mWindow);
}

bool
DecafSDLOpenGL::initialise(int width, int height)
{
   if (SDL_GL_LoadLibrary(NULL) != 0) {
      gCliLog->error("Failed to load OpenGL library: {}", SDL_GetError());
      return false;
   }

   SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

   // Set to OpenGL 4.5 core profile
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

   // Enable debug context
   if (decaf::config::gpu::debug) {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
   }

   // Create TV window
   mWindow = SDL_CreateWindow("Decaf",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      width, height,
      SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

   if (!mWindow) {
      gCliLog->error("Failed to create GL SDL window");
      return false;
   }

   SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

   // Create OpenGL context
   mContext = SDL_GL_CreateContext(mWindow);

   if (!mContext) {
      gCliLog->error("Failed to create Main OpenGL context: {}", SDL_GetError());
      return false;
   }

   mThreadContext = SDL_GL_CreateContext(mWindow);

   if (!mThreadContext) {
      gCliLog->error("Failed to create GPU OpenGL context: {}", SDL_GetError());
      return false;
   }

   SDL_GL_MakeCurrent(mWindow, mContext);

   // Setup decaf driver
   auto glDriver = decaf::createGLDriver();
   decaf_check(glDriver);
   mDecafDriver = reinterpret_cast<decaf::OpenGLDriver*>(glDriver);

   // Setup rendering
   initialiseContext();
   initialiseDraw();
   decaf::debugger::initialiseUiGL();

   ovrResult result = ovr_Initialize(nullptr);
   ovr_IdentifyClient("EngineName: Decaf\n"
     "EngineVersion: 0.0.1\n"
     "EngineEditor: false");

   result = ovr_Create(&hmdSession, &ovrLuid);

   if (OVR_FAILURE(result))
   {
     //common->Printf("\nFailed to initialize Oculus Rift.\n");
     ovr_Shutdown();
     return false;
   }
   ovr_SetTrackingOriginType(hmdSession, ovrTrackingOrigin_EyeLevel);
   ovr_RecenterTrackingOrigin(hmdSession);

   hmdDesc = ovr_GetHmdDesc(hmdSession);
   InitOculusTextures();

   // Start graphics thread
   if (!config::gpu::force_sync) {
      SDL_GL_SetSwapInterval(1);

      mGraphicsThread = std::thread{
         [this]() {
         SDL_GL_MakeCurrent(mWindow, mThreadContext);
         initialiseContext();
         mDecafDriver->run();
      } };
   } else {
      // Set the swap interval to 0 so that we don't slow
      //  down the GPU system when presenting...  The game should
      //  throttle our swapping automatically anyways.
      SDL_GL_SetSwapInterval(0);

      // Switch to the thread context, we automatically switch
      //  back when presenting a frame.
      SDL_GL_MakeCurrent(mWindow, mThreadContext);

      // Initialise the context
      initialiseContext();
   }

   return true;
}

void
DecafSDLOpenGL::shutdown()
{
   // Shut down the GPU
   if (!config::gpu::force_sync) {
      mDecafDriver->stop();
      mGraphicsThread.join();
   }
   ovr_DestroyTextureSwapChain(hmdSession, oculusSwapChain[0]);
   ovr_DestroyTextureSwapChain(hmdSession, oculusSwapChain[1]);

   ovr_Destroy(hmdSession);
   ovr_Shutdown();
}

void
DecafSDLOpenGL::renderFrame(Viewport &tv, Viewport &drc)
{
   if (!config::gpu::force_sync) {
      gl::GLuint tvBuffer = 0;
      gl::GLuint drcBuffer = 0;
      mDecafDriver->getSwapBuffers(&tvBuffer, &drcBuffer);
      drawScanBuffers(tv, tvBuffer, drc, drcBuffer);
   } else {
      mDecafDriver->syncPoll([&](unsigned int tvBuffer, unsigned int drcBuffer) {
         SDL_GL_MakeCurrent(mWindow, mContext);
         drawScanBuffers(tv, tvBuffer, drc, drcBuffer);
         SDL_GL_MakeCurrent(mWindow, mThreadContext);
      });
   }
}

decaf::GraphicsDriver *
DecafSDLOpenGL::getDecafDriver()
{
   return mDecafDriver;
}

#endif // DECAF_NOGL
