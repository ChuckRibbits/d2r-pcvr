#pragma once
namespace billboards {
void Install(void (*log)(const char*), bool (*active)());
// The cyclopean position used by the current stereo pair, in native world units.
void SetCenter(const float position[3]);
}
