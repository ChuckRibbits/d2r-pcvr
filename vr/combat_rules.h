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
}
