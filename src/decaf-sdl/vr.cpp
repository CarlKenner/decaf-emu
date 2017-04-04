#include "vr.h"

bool				hasHMD;
bool				hasOculusRift;
bool				vrTouching;

ovrSession			hmdSession;
ovrGraphicsLuid		ovrLuid;

ovrHmdDesc			hmdDesc;
ovrTextureSwapChain oculusSwapChain[2];
uint32_t				oculusFboId;
uint32_t				ocululsDepthTexID;
ovrSizei renderTarget;

ovrInputState vri;
ovrVector3f vrTouchPoint;

Matrix44 headRotation;
float headPosition[3];

bool g_new_tracking_frame;
ovrPosef g_eye_poses[2];
double predictedDisplayTime;

void VR_NewVRFrame()
{
   g_new_tracking_frame = true;
}

void UpdateOculusHeadTracking()
{
   ovrSessionStatus ss;
   ovr_GetSessionStatus(hmdSession, &ss);
   if (ss.ShouldRecenter)
      ovr_RecenterTrackingOrigin(hmdSession);
   if (ss.IsVisible)
   {

      predictedDisplayTime = ovr_GetPredictedDisplayTime(hmdSession, 0);
      ovrTrackingState s = ovr_GetTrackingState(hmdSession, predictedDisplayTime, true);

      OVR::Posef p = s.HeadPose.ThePose;
      OVR::Matrix4f m = OVR::Matrix4f(p.Rotation);
      for (int r = 0; r < 4; r++)
         for (int c = 0; c < 4; c++)
            headRotation.data[c * 4 + r] = m.M[r][c];
      g_eye_poses[0] = p;
      g_eye_poses[1] = g_eye_poses[0];
      headPosition[0] = p.Translation.x;
      headPosition[1] = p.Translation.y;
      headPosition[2] = p.Translation.z;
   }
   else
   {
      headRotation.setIdentity();
      headPosition[0] = headPosition[1] = headPosition[2] = 0;
   }
}

void VR_UpdateHeadTrackingIfNeeded()
{
   if (g_new_tracking_frame)
   {
      g_new_tracking_frame = false;
      UpdateOculusHeadTracking();
   }
}

