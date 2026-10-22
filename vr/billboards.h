#pragma once
namespace billboards {
void Install(void (*log)(const char*), bool (*active)());
// The cyclopean position used by the current stereo pair, in native world units.
void SetCenter(const float position[3]);
void ConfigurePortalCenters(bool enabled);
// The game camera's own view direction this frame (world, unit length). Unit
// overlays (auras) take its vertical part as their view direction.
void SetGameForward(const float forward[3]);
}
