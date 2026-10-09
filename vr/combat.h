#pragma once
#include <cstdint>

namespace combat {
void Configure(bool enabled);
void Install();
void Report();
// Only the native local unit identified by the controller aim hook. SDK player
// handles are opaque capabilities, NEVER native pointers.
void ObserveLocalPlayer(uintptr_t unit);
bool LocalPlayerId(uint32_t& id);
}
