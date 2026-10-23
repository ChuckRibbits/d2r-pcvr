#pragma once
#include <cstdint>

namespace projectiles {
// Optional, signature-checked native motion-bow adjustments. Ordinary attacks
// from other owners and all non-basic-arrow missile types retain native rules.
void Configure(bool motionControls, float arrowSpeed, bool directRelease);
void Install();
void Report();
bool Ready();
uintptr_t CreateVisual(uintptr_t params);
// Telemetry ("Frame missile flight:"): where the local hero's missiles end. The game thread.
void FollowFlights();
}
