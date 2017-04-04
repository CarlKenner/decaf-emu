#pragma once

#include "OVR_CAPI.h"
#include "extras/OVR_Math.h"
#include "Common/MathUtil.h"

extern bool				hasHMD;
extern bool				hasOculusRift;

extern ovrSession			hmdSession;
extern ovrGraphicsLuid		ovrLuid;

extern ovrHmdDesc			hmdDesc;
extern ovrTextureSwapChain oculusSwapChain[2];
extern uint32_t				oculusFboId;
extern uint32_t				ocululsDepthTexID;
extern ovrSizei renderTarget;

extern ovrInputState vri;
extern bool vrTouching;
extern ovrVector3f vrTouchPoint;

extern Matrix44 headRotation;
extern float headPosition[3];

extern ovrPosef g_eye_poses[2];
extern double predictedDisplayTime;

void VR_NewVRFrame();
void VR_UpdateHeadTrackingIfNeeded();