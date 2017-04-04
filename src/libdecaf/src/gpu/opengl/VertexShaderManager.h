#pragma once

#ifndef DECAF_NOGL

namespace gpu
{

namespace opengl
{

void SetProjectionConstants(float gmView[12], float gmProj[16], float gmViewProj[16], float gmInvView[12], float gEyePos[3], float gEyeVec[3], float gViewYVec[3]);

} // namespace opengl

} // namespace gpu

#endif // DECAF_NOGL
