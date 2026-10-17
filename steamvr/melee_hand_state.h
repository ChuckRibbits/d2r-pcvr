#pragma once
#include "melee_contact.h"
#include "attack_speed.h"

namespace framevr::melee {
// One instance per physical weapon; Classic's shared cadence lives outside it.
struct HandCombat {
    HitSet hits,landed,felt;
    attackspeed::Cadence recovery;
    uint64_t strokeSession=0,strokeId=0,cleaveSession=0,cleaveStroke=0;
    float strokeRecovery=1;unsigned cleaveLanded=0;
};
}
