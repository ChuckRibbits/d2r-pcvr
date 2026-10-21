#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace framevr::holybolt {
inline bool Skill(int id){return id==101;}
// Current compiled MonStats, audited against all owned text/binary rows.
// This only selects assist targets; native missile hit code owns damage/healing.
enum class Target {Ignore,Enemy,Friend};
inline Target AssistTarget(std::span<const uint8_t> row,bool hostile) {
    if(row.size()!=508)return Target::Ignore;
    const auto flags=row[0x3d];
    if(!(flags&0x80)||(flags&1))return Target::Ignore; // killable, not town NPC
    if(!hostile)return Target::Friend;
    return flags&0x38?Target::Enemy:Target::Ignore; // demon or either undead flag
}
// Native Collision assembles these four pointers before invoking its selected
// filter (3.3.93787, 0x4632d2). Never retain a borrowed table row across ticks.
struct CollisionArgs {uintptr_t game,missile,owner,row;};
static_assert(sizeof(CollisionArgs)==32&&offsetof(CollisionArgs,row)==24);
// Unlike legacy 32-bit callbacks, the current engine passes the data bank in
// RCX, target in RDX, and arguments in R8 (0x457d10). A two-argument call puts
// the argument pointer in the target register and silently rejects contacts.
using CollisionFilter=int(*)(uint8_t,uintptr_t,const CollisionArgs*);
inline bool CanCollide(CollisionFilter filter,uint8_t bank,uintptr_t target,const CollisionArgs& args) {
    return filter&&bank>=1&&bank<=3&&filter(bank,target,&args)!=0;
}
}
