#pragma once
#include <cstdint>

namespace combat {
void Configure(bool enabled);
// Calling thread only: true restores native hit/block/dodge rolls until reset.
// Returns the previous value so the caller can restore it.
bool NativeRolls(bool on);
// Native to-hit rolls for every local-player non-missile hit check (swings,
// trigger/button melee, melee skills). Missiles keep contact_hits.
void ConfigureMeleeRolls(bool native);
// Latest trigger/button melee hit check (outside physical swings), consumed
// once: its session and GetTickCount64 time. False when there is none.
bool TakeButtonMelee(uint64_t& session,uint64_t& at);
// Unleashed physical guard. A monster hit that passes its native to-hit roll
// is blocked by a raised shield facing the attacker, not by the block stat.
// parryMs 0 disables perfect parry. Outside first person, menus or without a
// shield (eligible=false) the native block roll is kept.
void ConfigureGuard(bool enabled,uint32_t parryMs,bool trace=false); // trace: [debug] guard_trace
// [gameplay] enemies_always_hit: monsters' to-hit rolls against the local player always connect.
void ConfigureEnemiesAlwaysHit(bool on);
// Game thread (melee.cpp): a physical swing's native attack on this monster id
// is running (NoSwing UINT32_MAX = none). Returns the previous value.
uint32_t PhysicalSwing(uint32_t target);
// Game thread: a parry flinched this monster; its next physical hit within the window is a riposte.
void NoteRiposte(uint32_t monster);
// [gameplay] riposte_bonus (% extra damage), riposte_ms, guard_swing_damage (% of
// damage dealt with the shield raised), riposte_sound_file (empty: the built-in sound; else a WAV, next to the ini unless a full path).
// guard_swing_step: % less on each further hit while the shield stays raised (to 0).
// riposte_volume: % on top of the game's own Master x Combat volume (Settings.json).
void ConfigureRiposte(float bonus,uint32_t windowMs,float raisedFactor,float raisedStep,float volume,const wchar_t* soundFile);
// UI thread, every tick: where the headset looks on the ground (render X/Z, the way
// the guard faces); the shield hand from the head in the view's turn (metres, x right,
// y up, z back) decides raised and the parry swing. leftShield: the shield's hand.
// posed: the shield is posed on the tracked hand this frame.
void PublishGuard(uint64_t session,bool eligible,bool posed,const float view[3],float unitsPerMetre,
                  const float hand[3],bool handOk,bool leftShield);
void Install();
void Report();
// The UI-thread local-player accessor is authoritative across area changes;
// controller aim can also refresh identity. SDK handles are never pointers.
void ObserveLocalPlayer(uintptr_t unit);
bool ObserveUiPlayer(uintptr_t unit,uint64_t session);
bool LocalPlayerUnit(uintptr_t& unit);
bool LocalPlayerId(uint32_t& id);
}
