#pragma once
#include <glbinding/gl/gl.h>
#include "OVR_CAPI_GL.h"

struct TextureBuffer
{
#if OVR_PRODUCT_VERSION >= 1
#define ovrHmd ovrSession
  ovrTextureSwapChain TextureChain;
#else
  ovrSwapTextureSet* TextureSet;
#endif
  gl::GLuint texId;
  gl::GLuint fboId;
  ovrSizei texSize;
  ovrRecti viewport;
  ovrHmd hmdSession;

  TextureBuffer()
  {
  }

  TextureBuffer(ovrHmd hmd, bool rendertarget, bool displayableOnHmd, int width, int height,
    int mipLevels, unsigned char* data)
  {
#if OVR_PRODUCT_VERSION >= 1
    TextureChain = nullptr;
    texId = 0;
    fboId = 0;
#endif
    hmdSession = hmd;
    texSize.w = width;
    texSize.h = height;
    viewport.Pos.x = 0;
    viewport.Pos.y = 0;
    viewport.Size = texSize;

    if (displayableOnHmd)
    {
      int swapChainLength = 0;
#if OVR_PRODUCT_VERSION >= 1
      ovrResult res;
      ovrTextureSwapChainDesc desc = {};
      desc.Type = ovrTexture_2D;
      desc.ArraySize = 1;
      desc.Width = width;
      desc.Height = height;
      desc.MipLevels = 1;
      desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
      desc.SampleCount = 1;
      desc.MiscFlags = 0;
      desc.BindFlags = 0;
      desc.StaticImage = ovrFalse;

      res = ovr_CreateTextureSwapChainGL(hmd, &desc, &TextureChain);
      ovr_GetTextureSwapChainLength(hmd, TextureChain, &swapChainLength);
      if (!OVR_SUCCESS(res))
      {
        ovrErrorInfo e;
        ovr_GetLastErrorInfo(&e);
        //PanicAlert("ovr_CreateTextureSwapChainGL(hmd, OVR_FORMAT_R8G8B8A8_UNORM_SRGB, %d, %d)=%d "
        //  "failed\n%s",
        //  size.w, size.h, res, e.ErrorString);
        return;
      }
#elif OVR_MAJOR_VERSION >= 7
      ovr_CreateSwapTextureSetGL(hmd, gl::GL_SRGB8_ALPHA8, size.w, size.h, &TextureSet);
      swapChainLength = TextureSet->TextureCount;
#else
      ovrHmd_CreateSwapTextureSetGL(hmd, gl::GL_RGBA, size.w, size.h, &TextureSet);
      swapChainLength = TextureSet->TextureCount;
#endif
      for (int i = 0; i < swapChainLength; ++i)
      {
#if OVR_PRODUCT_VERSION >= 1
        gl::GLuint chainTexId;
        ovr_GetTextureSwapChainBufferGL(hmd, TextureChain, i, &chainTexId);
        gl::glBindTexture(gl::GL_TEXTURE_2D, chainTexId);
#else
        ovrGLTexture* tex = (ovrGLTexture*)&TextureSet->Textures[i];
        gl::glBindTexture(gl::GL_TEXTURE_2D, tex->OGL.TexId);
#endif

        if (rendertarget)
        {
          gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR);
          gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
          gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_CLAMP_TO_EDGE);
          gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_CLAMP_TO_EDGE);
        }
        else
        {
          gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR_MIPMAP_LINEAR);
          gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
          gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_REPEAT);
          gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_REPEAT);
        }
      }
    }
    else
    {
      gl::glGenTextures(1, &texId);
      gl::glBindTexture(gl::GL_TEXTURE_2D, texId);

      if (rendertarget)
      {
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_CLAMP_TO_EDGE);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_CLAMP_TO_EDGE);
      }
      else
      {
        glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
        glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_REPEAT);
        glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_REPEAT);
      }

#if OVR_PRODUCT_VERSION >= 1
      gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_SRGB8_ALPHA8, texSize.w, texSize.h, 0, gl::GL_RGBA,
        gl::GL_UNSIGNED_BYTE, data);
#else
      gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA, texSize.w, texSize.h, 0, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE,
        data);
#endif
    }

    if (mipLevels > 1)
    {
      gl::glGenerateMipmap(gl::GL_TEXTURE_2D);
    }

    gl::glGenFramebuffers(1, &fboId);
  }

  ovrSizei GetSize(void) const { return texSize; }
  void SetAndClearRenderSurface()
  {
#if OVR_PRODUCT_VERSION >= 1
    gl::GLuint curTexId;
    if (TextureChain)
    {
      int curIndex;
      ovr_GetTextureSwapChainCurrentIndex(hmdSession, TextureChain, &curIndex);
      ovr_GetTextureSwapChainBufferGL(hmdSession, TextureChain, curIndex, &curTexId);
    }
    else
    {
      curTexId = texId;
    }

    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, fboId);
    gl::glFramebufferTexture2D(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, gl::GL_TEXTURE_2D, curTexId, 0);

    gl::glViewport(viewport.Pos.x, viewport.Pos.y, viewport.Size.w, viewport.Size.h);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glEnable(GL_FRAMEBUFFER_SRGB);
#else
    ovrGLTexture* tex = (ovrGLTexture*)&TextureSet->Textures[TextureSet->CurrentIndex];

    glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->OGL.TexId, 0);

    glViewport(0, 0, texSize.w, texSize.h);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
  }

  void UnsetRenderSurface()
  {
    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, fboId);
    gl::glFramebufferTexture2D(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, gl::GL_TEXTURE_2D, 0, 0);
    gl::glFramebufferTexture2D(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, gl::GL_TEXTURE_2D, 0, 0);
  }

  void Commit()
  {
#if OVR_PRODUCT_VERSION >= 1
    if (TextureChain)
    {
      ovr_CommitTextureSwapChain(hmdSession, TextureChain);
    }
#endif
  }
};
