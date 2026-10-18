#pragma once
namespace offlinedifficulty {
// 0 uses the game's own setting; 1..8 replaces its effective offline count.
// Settings/update thread only. Does not write the game's global preferences.
void Configure(int players, bool vr);
void Install();
void Report();
}
