#pragma once
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
}
