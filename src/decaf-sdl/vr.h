#pragma once

#include "OVR_CAPI.h"
#include "extras/OVR_Math.h"

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

