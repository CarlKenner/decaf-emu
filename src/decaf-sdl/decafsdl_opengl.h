#pragma once
#ifndef DECAF_NOGL
#include "decafsdl_graphics.h"
#include "libdecaf/decaf.h"
#include "libdecaf/decaf_opengl.h"
#include <SDL.h>
#include <glbinding/gl/gl.h>
#include "..\LibOVR\Include\OVR_CAPI.h"
#include "..\LibOVR\Include\OVR_CAPI_GL.h"

class DecafSDLOpenGL : public DecafSDLGraphics
{
public:
   DecafSDLOpenGL();
   ~DecafSDLOpenGL() override;

   void InitOculusTextures();

   bool
   initialise(int width,
              int height) override;

   void
   shutdown() override;

   void
   renderFrame(Viewport &tv,
               Viewport &drc) override;

   decaf::GraphicsDriver *
   getDecafDriver() override;

protected:
   void
   drawScanBuffer(gl::GLuint object);

   void
   drawScanBuffers(Viewport &tvViewport,
                   gl::GLuint tvBuffer,
                   Viewport &drcViewport,
                   gl::GLuint drcBuffer);

   void
   initialiseContext();

   void
   initialiseDraw();

protected:
   std::thread mGraphicsThread;
   decaf::OpenGLDriver *mDecafDriver = nullptr;

   SDL_GLContext mContext = nullptr;
   SDL_GLContext mThreadContext = nullptr;

   gl::GLuint mVertexProgram;
   gl::GLuint mPixelProgram;
   gl::GLuint mPipeline;
   gl::GLuint mVertArray;
   gl::GLuint mVertBuffer;
   gl::GLuint mSampler;
   
   gl::GLfloat mBackgroundColour[3];
public:
  bool				hasHMD;
  bool				hasOculusRift;

  ovrSession			hmdSession;
  ovrGraphicsLuid		ovrLuid;

  ovrHmdDesc			hmdDesc;
  ovrTextureSwapChain oculusSwapChain[2];
  uint32_t				oculusFboId;
  uint32_t				ocululsDepthTexID;
  ovrSizei renderTarget;
};

#endif // DECAF_NOGL
