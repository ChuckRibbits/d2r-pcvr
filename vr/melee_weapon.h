#pragma once
#include "../steamvr/dual_wield.h"

namespace meleeweapon {
void Install();
bool Ready();
// All native pointers stay on their owning thread. Validates both current
// rendered weapons against the sampled inventory IDs before selecting one.
uintptr_t Select(uintptr_t player,int hand,const framevr::dualwield::Equipment& equipment);
class Scope {
    uintptr_t player=0,previousPlayer=0,previousWeapon=0;
public:
    Scope(uintptr_t player,uintptr_t weapon);
    ~Scope();
    Scope(const Scope&)=delete;
    Scope& operator=(const Scope&)=delete;
};
}
