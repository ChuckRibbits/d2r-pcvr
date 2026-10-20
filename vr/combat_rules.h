#pragma once
#include <cmath>
#include <cstdint>

namespace combat {
struct Seed {uint32_t low=0,high=0;};
inline Seed AdvanceSeed(Seed seed) {
    const uint64_t next=uint64_t(seed.low)*0x6ac690c5u+seed.high;
    return {uint32_t(next),uint32_t(next>>32)};
}
inline bool PlayerContact(uint32_t localId,uint32_t attackerType,uint32_t attackerId,uint32_t defenderType) {
    return localId!=UINT32_MAX&&attackerType==0&&attackerId==localId&&defenderType==1;
}
// A monster (or its missile, resolved to the native owner) striking the local player.
inline bool IncomingContact(uint32_t localId,uint32_t attackerType,uint32_t defenderType,uint32_t defenderId) {
    return localId!=UINT32_MAX&&attackerType==1&&defenderType==0&&defenderId==localId;
}
// Scratch seed fed to the block roll: advances to 0 (always under any block
// rate) or 99 (never under D2's 75% cap). Same inline step as the hit roll.
inline Seed GuardRoll(bool block) {return block?Seed{0,0}:Seed{0,99};}
// Unleashed swing damage: attack rating scales a landed swing instead of
// deciding a miss. chance: the native, already clamped to-hit % (5..95).
// floor: the share kept at zero chance ([gameplay] ar_damage_floor; 0 = the
// raw chance).
inline float ArFactor(int chance,float floor) {
    const float c=float(chance<0?0:chance>100?100:chance)/100.f;
    const float f=floor<0?0.f:floor>1?1.f:floor;
    return f+(1-f)*c;
}
// Unleashed incoming defense: a monster's to-hit roll against the local player
// always connects, and the native chance it would have had (its attack rating
// against the player's Defense, clamped 5..95) scales the whole landed attack,
// elemental riders included. Applied at the final life write, after damage
// reduction, resistances and absorb, so the expected damage equals native
// (chance x a landed hit) without the hit-or-miss variance. No floor.
inline float DefenseFactor(int chance) {return ArFactor(chance,0);}
// Unleashed swing damage: how recovered a stroke is - the time since the
// previous landed stroke over the native attack cycle, raised to `curve`, at
// most 1. curve 1 is damage-per-second neutral (twice the strokes at half the
// damage); 2 makes flailing lose damage (twice the rate = half the DPS).
// grace (0..1]: the share of the cycle after which a stroke counts as fully
// recovered (1 = none). An unknown cycle (0) or no previous stroke
// (UINT64_MAX) is fully recovered.
inline float RecoveryFactor(uint64_t sinceMs,uint32_t cycleMs,float curve=1,float grace=1) {
    if(!cycleMs||sinceMs==UINT64_MAX)return 1;
    const float g=grace>.05f&&grace<1?grace:1;
    const float x=float(sinceMs)/(float(cycleMs)*g);
    if(x>=1)return 1;
    const float k=curve>=1&&curve<=4?curve:1;
    return k==1?x:k==2?x*x:float(std::pow(double(x),double(k)));
}
// Unleashed cleave falloff: the n-th creature (0 = first) one stroke lands on
// deals 1 - step*n of its damage, never below floor. step 0 = no falloff.
inline float CleaveFactor(unsigned index,float step,float floor) {
    const float s=step>0?step:0.f,f=floor<0?0.f:floor>1?1.f:floor;
    const float v=1-s*float(index);
    return v<f?f:v>1?1.f:v;
}
}
