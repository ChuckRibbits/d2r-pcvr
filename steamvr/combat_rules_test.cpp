#include "../vr/combat_rules.h"
#undef NDEBUG
#include <cassert>
#include <cstdio>

int main() {
    using namespace combat;
    assert(PlayerContact(0,0,0,1)); // id zero is a valid player
    assert(PlayerContact(42,0,42,1));
    assert(!PlayerContact(UINT32_MAX,0,UINT32_MAX,1));
    assert(!PlayerContact(42,1,42,0)); // incoming attacks
    assert(!PlayerContact(42,1,42,1)); // summons/monsters with overlapping ids
    assert(!PlayerContact(42,0,41,1)); // another player
    assert(!PlayerContact(42,0,42,0)); // PvP
    assert(!PlayerContact(42,3,42,1)); // projectile must resolve its native owner
    assert(!PlayerContact(42,0,42,2)); // scenery is not a damage target
    assert(IncomingContact(42,1,0,42));
    assert(IncomingContact(0,1,0,0));
    assert(!IncomingContact(42,1,0,41)); // another player is struck
    assert(!IncomingContact(42,0,0,42)); // PvP is not a guard
    assert(!IncomingContact(UINT32_MAX,1,0,UINT32_MAX));
    assert(!IncomingContact(42,3,0,42)); // missiles resolve their native owner first
    for(int rate=1;rate<=75;++rate) {
        assert(AdvanceSeed(GuardRoll(true)).low%100<uint32_t(rate));
        assert(!(AdvanceSeed(GuardRoll(false)).low%100<uint32_t(rate)));
    }
    // The native stream after the intercepted roll must be identical, so the
    // following damage/proc roll is unchanged. Exercise carries and overflow.
    Seed seed{0xffffffffu,0xffffffffu};
    for(int i=0;i<10000;++i) {
        const uint64_t native=uint64_t(seed.low)*0x6ac690c5u+seed.high;
        const Seed advanced=AdvanceSeed(seed);
        assert(advanced.low==uint32_t(native)&&advanced.high==uint32_t(native>>32));
        const auto nextNative=uint64_t(uint32_t(native))*0x6ac690c5u+uint32_t(native>>32);
        assert(AdvanceSeed(advanced).low==uint32_t(nextNative));
        for(int chance=5;chance<=95;++chance)assert(AdvanceSeed({}).low%100<uint32_t(chance));
        seed=advanced;
    }
    std::puts("Combat rules: local ownership, guaranteed accuracy and subsequent random-stream tests passed");
}
