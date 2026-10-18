#pragma once
namespace roofmaterials {
// Install before world assets load; retry while encrypted code is unavailable.
// Materials are cached by the game, so changing the gate requires a restart.
void Install(void (*log)(const char*), bool (*nativeVr)());
}
