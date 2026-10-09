#pragma once
#include <cstdint>

// Called by the existing game-side stereo draw hook and ReShade integration.
// The OpenXR session uses the game's D3D12 device/queue through Proton.
namespace framevr {
using LogFn = void (*)(const char*);
using PanelFn = bool (*)();
void Register(LogFn log, PanelFn wantsPanel, PanelFn inWorld, PanelFn toggleMenus);
void ConfigureDeferredWait(bool enabled);
void ConfigureSymmetricProjection(bool enabled);
void ConfigureEyeTracking(bool enabled);
struct FrameTiming {
    double waitFrameMs=0,waitGpuMs=0,submitMs=0,maxPipelineMs=0,locateMs=0,inputMs=0,releaseMs=0;
    double acquireMs[2]{},copySubmitMs[2]{},gpuSceneMs[2]{},gpuEffectsMs[2]{},gpuCopyMs[2]{};
    uint32_t pipelines=0;bool gpuValid=false;
};
FrameTiming LastTiming();
void Shutdown();
void BeginPair();
void BeginEye(int eye);
// Diagnostic only: F8 saves the next complete gameplay pair via ReShade.
void RequestStereoCapture();
void EndPair();
void ResetHeight();
void Recenter();
bool TakeWorldRecenter();
bool Active();
bool View(float out[16], const float hero[3], float eyeHeight,
          float userEyeHeight, float bodyHeading, float recenterYaw, int eye);
bool Projection(float out[16], float nearWorld, int eye);
}
