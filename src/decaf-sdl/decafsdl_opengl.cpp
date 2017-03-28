#ifndef DECAF_NOGL

#include "clilog.h"
#include <common/decaf_assert.h>
#include "config.h"
#include "decafsdl_opengl.h"
#include <string>
#include <glbinding/Binding.h>
#include <glbinding/Meta.h>
#include "..\LibOVR\Include\OVR_CAPI.h"
#include "..\LibOVR\Include\OVR_CAPI_GL.h"

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
  renderTarget = ovr_GetFovTextureSize(hmdSession, (ovrEyeType)0, hmdDesc.DefaultEyeFov[0], 1.0f);
  ovrTextureSwapChainDesc desc = {};
    desc.Type = ovrTexture_2D;
    desc.ArraySize = 1;
    desc.Width = renderTarget.w;
    desc.Height = renderTarget.h;
    desc.MipLevels = 1;
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.SampleCount = 1;
    desc.StaticImage = ovrFalse;


    // create the swap texture sets 
    if (ovr_CreateTextureSwapChainGL(hmdSession, &desc, &oculusSwapChain[0]) != ovrSuccess ||
      ovr_CreateTextureSwapChainGL(hmdSession, &desc, &oculusSwapChain[1]) != ovrSuccess)
    {
      //common->Warning("iVr::HMDInitializeDistortion unable to create OVR swap texture set.\n VR mode is DISABLED.\n");
      //game->isVR = false;

    }

    unsigned int texId = 0;
    int length = 0;

    for (int j = 0; j < 2; j++)
    {
      ovr_GetTextureSwapChainLength(hmdSession, oculusSwapChain[j], &length);
      for (int i = 0; i < length; ++i)
      {
        ovr_GetTextureSwapChainBufferGL(hmdSession, oculusSwapChain[j], 0, &texId);
        //oculusSwapChainTexId[j] = texId;

        gl::glBindTexture(gl::GL_TEXTURE_2D, texId);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_CLAMP_TO_EDGE);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_CLAMP_TO_EDGE);
      }
    }

    ovr_CommitTextureSwapChain(hmdSession, oculusSwapChain[0]);
    ovr_CommitTextureSwapChain(hmdSession, oculusSwapChain[1]);

    gl::glGenFramebuffers(1, &oculusFboId);
    gl::glGenTextures(1, &ocululsDepthTexID);

    gl::glBindTexture(gl::GL_TEXTURE_2D, ocululsDepthTexID);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_CLAMP_TO_EDGE);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_CLAMP_TO_EDGE);

    gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_DEPTH_COMPONENT24, renderTarget.w, renderTarget.h, 0, gl::GL_DEPTH_COMPONENT, gl::GL_UNSIGNED_INT, NULL);

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

   gl::GLuint curTexId;
   int curIndex;

   ovr_GetTextureSwapChainCurrentIndex(hmdSession, oculusSwapChain[0], &curIndex);
   ovr_GetTextureSwapChainBufferGL(hmdSession, oculusSwapChain[0], curIndex, &curTexId);

   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, oculusFboId);
   gl::glFramebufferTexture2D(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, gl::GL_TEXTURE_2D, curTexId, 0);
   gl::glFramebufferTexture2D(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, gl::GL_TEXTURE_2D, ocululsDepthTexID, 0);

   ovrRecti viewport;
   viewport.Pos.x = 0;
   viewport.Pos.y = 0;
   viewport.Size = renderTarget;
   gl::glViewport(0, 0, renderTarget.w, renderTarget.h);
   gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
   if (drawTV)
   {
     drawScanBuffer(tvBuffer);
   }
   else if (drawDRC)
   {
     drawScanBuffer(drcBuffer);
   }

   ovr_GetTextureSwapChainCurrentIndex(hmdSession, oculusSwapChain[1], &curIndex);
   ovr_GetTextureSwapChainBufferGL(hmdSession, oculusSwapChain[1], curIndex, &curTexId);

   // Submit frame with one layer we have.

   ovr_CommitTextureSwapChain(hmdSession, oculusSwapChain[0]);
   ovr_CommitTextureSwapChain(hmdSession, oculusSwapChain[1]);


   ovrLayerQuad lg, lg2;
   lg.Header.Type = ovrLayerType_Quad;
   lg.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
   lg.ColorTexture = oculusSwapChain[0];
   lg.Viewport = viewport;
   lg.QuadSize.x = 3.0f; // metres
   lg.QuadSize.y = lg.QuadSize.x * 3.0f / 4.0f; // metres
   lg.QuadPoseCenter.Position.x = 0; // metres
   lg.QuadPoseCenter.Position.y = 0; // metres
   lg.QuadPoseCenter.Position.z = -1.5f; // metres (negative means in front of us)
   lg.QuadPoseCenter.Orientation.w = 1;
   lg.QuadPoseCenter.Orientation.x = 0;
   lg.QuadPoseCenter.Orientation.y = 0;
   lg.QuadPoseCenter.Orientation.z = 0;

   // draw another one behind us
   lg2 = lg;
   lg2.QuadPoseCenter.Orientation.w = 0;
   lg2.QuadPoseCenter.Orientation.y = 1;
   lg2.QuadPoseCenter.Position.z = -lg.QuadPoseCenter.Position.z;

   ovrLayerHeader* LayerList[2];
   LayerList[0] = &lg.Header;
   LayerList[1] = &lg2.Header;
   //common->Printf( "Frame Submitting 2D frame # %d\n", idLib::frameNumber );
   ovrResult result = ovr_SubmitFrame(hmdSession, 0, NULL, LayerList, 2);
   if (result == ovrSuccess_NotVisible)
   {
   }
   else if (result == ovrError_DisplayLost)
   {
     //common->Warning("Vr_GL.cpp HMDRender : Display Lost when submitting oculus layer.\n");
   }
   else if (OVR_FAILURE(result))
   {
     //common->Warning("Vr_GL.cpp HMDRender : Failed to submit oculus layer. (result %d) \n", result);
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
