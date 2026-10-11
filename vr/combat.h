#pragma once
#include <cstdint>

namespace combat {
void Configure(bool enabled);
void Install();
void Report();
// The UI-thread local-player accessor is authoritative across area changes;
// controller aim can also refresh identity. SDK handles are never pointers.
void ObserveLocalPlayer(uintptr_t unit);
bool ObserveUiPlayer(uintptr_t unit,uint64_t session);
bool LocalPlayerUnit(uintptr_t& unit);
bool LocalPlayerId(uint32_t& id);
}
