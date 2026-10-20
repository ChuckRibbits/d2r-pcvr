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
    // Unleashed swing damage (compared with a tolerance: float maths may contract).
    const auto near=[](float a,float b){return a>b-1e-4f&&a<b+1e-4f;};
    // Attack rating: raw chance by default, a floor lifts the bottom only.
    assert(near(ArFactor(95,0),.95f)&&near(ArFactor(5,0),.05f)&&near(ArFactor(50,0),.5f));
    assert(near(ArFactor(0,.25f),.25f)&&near(ArFactor(100,.25f),1)&&near(ArFactor(50,.5f),.75f));
    assert(near(ArFactor(-3,0),0)&&near(ArFactor(140,0),1)&&near(ArFactor(50,-1),.5f)&&near(ArFactor(50,2),1));
    // Incoming defense: the raw native chance, no floor (5% chance = 5% of the hit).
    assert(near(DefenseFactor(5),.05f)&&near(DefenseFactor(95),.95f)&&near(DefenseFactor(60),.6f));
    assert(near(DefenseFactor(-1),0)&&near(DefenseFactor(120),1));
    // Recovery, linear (curve 1).
    assert(RecoveryFactor(UINT64_MAX,600)==1.f); // first stroke
    assert(RecoveryFactor(300,0)==1.f);          // unknown cycle never penalizes
    assert(near(RecoveryFactor(0,600),0)&&near(RecoveryFactor(150,600),.25f)&&near(RecoveryFactor(300,600),.5f));
    assert(RecoveryFactor(600,600)==1.f&&RecoveryFactor(5000,600)==1.f);
    // Curve 2: flailing at twice the rate lands half the damage per second.
    assert(near(RecoveryFactor(300,600,2),.25f)&&near(2*RecoveryFactor(300,600,2),.5f));
    assert(near(RecoveryFactor(150,600,2),.0625f)&&RecoveryFactor(600,600,2)==1.f&&RecoveryFactor(UINT64_MAX,600,2)==1.f);
    assert(RecoveryFactor(300,0,2)==1.f);
    // Grace: from that share of the cycle a stroke is fully recovered.
    assert(RecoveryFactor(520,600,2,.85f)==1.f&&near(RecoveryFactor(510,600,2,.85f),1)&&near(RecoveryFactor(255,600,2,.85f),.25f));
    assert(near(RecoveryFactor(300,600,2,1),.25f)&&near(RecoveryFactor(300,600,2,0),.25f)); // out-of-range grace = none
    assert(near(RecoveryFactor(300,600,9),.5f));  // out-of-range curve falls back to linear
    // Cleave falloff: 100, 75, 50, 25 then the floor; no step = no falloff.
    assert(near(CleaveFactor(0,.25f,.25f),1)&&near(CleaveFactor(1,.25f,.25f),.75f)&&near(CleaveFactor(2,.25f,.25f),.5f));
    assert(near(CleaveFactor(3,.25f,.25f),.25f)&&near(CleaveFactor(9,.25f,.25f),.25f));
    assert(near(CleaveFactor(5,0,.25f),1)&&near(CleaveFactor(5,-1,.25f),1)&&near(CleaveFactor(4,.25f,0),0));
    std::puts("Combat rules: local ownership, guaranteed accuracy, subsequent random-stream and swing damage tests passed");
}
