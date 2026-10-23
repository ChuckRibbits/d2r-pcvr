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
// Telemetry ("Frame wall trace:"): a Bone Wall or Bone Prison cast at x,y
// (cells; point false: the hero's place, the game aims it). Any thread.
void WallCast(int skill,float x,float y,bool point);
// Its window, every 100 ms at most (targetlock's scan). The game thread.
void FollowWalls();
}
