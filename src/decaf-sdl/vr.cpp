#include "vr.h"

bool				hasHMD;
bool				hasOculusRift;

ovrSession			hmdSession;
ovrGraphicsLuid		ovrLuid;

ovrHmdDesc			hmdDesc;
ovrTextureSwapChain oculusSwapChain[2];
uint32_t				oculusFboId;
uint32_t				ocululsDepthTexID;
ovrSizei renderTarget;

ovrInputState vri;