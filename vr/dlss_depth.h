#pragma once
namespace dlssdepth {
// Retry with the other native hooks until the renderer code is available.
void Install(void (*log)(const char*), bool (*active)());
}
