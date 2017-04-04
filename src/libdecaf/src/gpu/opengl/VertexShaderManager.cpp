#ifndef DECAF_NOGL

#include "decaf_config.h"
#include "gpu/glsl2/glsl2_translate.h"
#include "gpu/gpu_utilities.h"
#include "gpu/latte_registers.h"
#include "gpu/microcode/latte_disassembler.h"
#include "opengl_constants.h"
#include "opengl_driver.h"
#include "decaf-sdl/vr.h"

#include <common/debuglog.h>
#include <common/decaf_assert.h>
#include <common/log.h>
#include <common/murmur3.h>
#include <common/platform_dir.h>
#include <common/strutils.h>
#include <common/MathUtil.h>
#include <fstream>
#include <glbinding/gl/gl.h>
#include <libcpu/mem.h>
#include <spdlog/spdlog.h>

namespace gpu
{

namespace opengl
{

float fUnitsPerMetre = 1.0f;

void PrintMatrix3x4(const char *name, float m[12])
{
   debugPrint(fmt::format("{}[{:.2f}, {:.2f}, {:.2f},  {:.1f}] [{:.2f}, {:.2f}, {:.2f},  {:.1f}] [{:.2f}, {:.2f}, {:.2f},  {:.1f}]", name, m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11]));
}

void PrintMatrix4x4(const char *name, float m[16])
{
   debugPrint(fmt::format("{}[{:.2f}, {:.2f}, {:.2f},  {:.2f}] [{:.2f}, {:.2f}, {:.2f},  {:.2f}] [{:.2f}, {:.2f}, {:.2f},  {:.2f}] [{:.2f}, {:.2f}, {:.2f},  {:.2f}]", name, m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]));
}

void PrintVec3(const char *name, float v[3])
{
   debugPrint(fmt::format("{}[{:.2f}, {:.2f}, {:.2f}]", name, v[0], v[1], v[2]));
}

void SetProjectionConstants(float gmView[12], float gmProj[16], float gmViewProj[16], float gmInvView[12], float gEyePos[3], float gEyeVec[3], float gViewYVec[3])
{
   //std::string s;
   float camPos[3] = { gmInvView[0 * 4 + 3], gmInvView[1 * 4 + 3], gmInvView[2 * 4 + 3] };
   Matrix44 view, proj, viewProj, invView;
   Matrix44::Set3x4(view, gmView);
   Matrix44::Set(proj, gmProj);
   //Matrix44::Set(viewProj, gmViewProj);
   //float hFOV = 2 * atan(1.0f / gmProj[0 * 4 + 0]) * 180.0f / 3.1415926535f;
   //float vFOV = 2 * atan(1.0f / gmProj[1 * 4 + 1]) * 180.0f / 3.1415926535f;
   //debugPrint(fmt::format("{:.2f} deg x {:.2f} deg Aspect 16:{}, near {}, far {}", proj.hFOV(), proj.vFOV(), 16 / proj.aspectRatio(), proj.openglNear(), proj.openglFar() ));

   //PrintMatrix3x4("view", view.data);
   //PrintMatrix4x4("proj", gmProj);
   //PrintMatrix4x4("gmViewProj", gmViewProj);
   //PrintMatrix3x4("gmInvView", gmInvView);
   //PrintVec3("camPos", camPos);
   //PrintVec3("gEyePos", gEyePos);
   //PrintVec3("gEyeVec", gEyeVec);
   //PrintVec3("gViewYVec", gViewYVec); // same as row 2 of gmInvView, roughly 0, 1, 0

   ovrMatrix4f mOculus = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[1], proj.openglNear(), proj.openglFar(), ovrProjection_ClipRangeOpenGL);
   Matrix44 oculus;
   for (int r = 0; r < 4; r++)
      for (int c = 0; c < 4; c++)
         oculus.data[r * 4 + c] = mOculus.M[r][c];
   PrintMatrix4x4("oculus", oculus.data);
   PrintMatrix4x4("proj", proj.data);

   proj.xx = oculus.xx; proj.zx = oculus.zx;
   proj.yy = oculus.yy; proj.zy = oculus.zy;
   


   //proj.yy = proj.xx;
   //view = view * headRotation;
   VR_UpdateHeadTrackingIfNeeded();
   Matrix44 headPositionTracking;
   float v[3];
   v[0] = -headPosition[0] * fUnitsPerMetre;
   v[1] = -headPosition[1] * fUnitsPerMetre;
   v[2] = -headPosition[2] * fUnitsPerMetre;
   Matrix44::Translate(headPositionTracking, v);

   proj = headPositionTracking * headRotation * proj;


   viewProj = view * proj;
   memcpy(gmView, view.data, 12 * sizeof(float));
   memcpy(gmProj, proj.data, 16 * sizeof(float));
   memcpy(gmViewProj, viewProj.data, 16 * sizeof(float));
   invView = view.simpleInverse();
   memcpy(gmInvView, invView.data, 12 * sizeof(float));
   // eyePos seems to just be gibberish, but we can set eyeVec
   gEyeVec[0] = -view.xz;
   gEyeVec[1] = -view.yz;
   gEyeVec[2] = -view.zz;
   gViewYVec[0] = view.yx;
   gViewYVec[1] = view.yy;
   gViewYVec[2] = view.yz;

}

} // namespace opengl

} // namespace gpu

#endif // DECAF_NOGL
