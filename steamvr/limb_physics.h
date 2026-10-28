#pragma once
#include "limb_rig.h"
#include <array>
#include <memory>

// Bodies that give way (limb_rig.h): each body a powered ragdoll in its own
// Box3D world. Its limbs are capsules joined at the bones' joints; springs at
// the joints hold the animated pose between limbs, and a spring holds the
// root limb to where the animation has it. Weapons and fists are kinematic
// capsules that push the limbs aside. The bodies are simulated in the
// animation's frame: every new pose carries each limb with it, keeping only
// its offset, so a monster that runs or swings does not lag behind its own
// animation; only pushes move a limb off it.
namespace framevr::limbs {
// A held weapon, shield or fist: base to tip in world metres. Slots as
// bottles::Collider: right hand, left hand, shield.
struct Weapon {bool active=false;Vec base{},tip{};float radius=0;};
inline constexpr int Weapons=3;
// A weapon that pushed a body: for the hand's tick and the log.
struct Touch {uint32_t entity=0;Kind kind=Kind::None;int slot=-1;float speed=0;};
inline constexpr unsigned MaxTouches=8;
inline constexpr double Step=1/120.;
inline constexpr float WakeReach=.6f;   // metres: a weapon this near a body's limbs brings it to life
inline constexpr double Unseen=.4;      // seconds a body may go without a pose before it goes
inline constexpr double RestTime=1.5;   // seconds back on its animation, out of reach, before it goes
inline constexpr float MaxTurn=1.2f;    // radians a limb may bend off its animation
inline constexpr float MaxShift=.3f;    // metres the whole body may be shoved
inline constexpr float Recentre=20;     // metres from the physics origin before it moves
class World {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    World();
    ~World();
    World(const World&)=delete;
    World& operator=(const World&)=delete;
    bool Ready() const;
    // The bodies near the player as posed now (Capture). A simulated body is
    // carried to its new pose; one unseen for Unseen seconds goes.
    void Bodies(const Rig* rigs,unsigned count,double time);
    // Where the weapons are now; the next Advance moves them there.
    void Hands(const std::array<Weapon,Weapons>&);
    // Fixed steps to time. A body comes to life when a weapon nears it and
    // goes once it is back on its animation with no weapon near.
    void Advance(double time,Vec player);
    // Every simulated body's offsets from its animation, at the latest step.
    unsigned Reactions(std::array<Reaction,MaxBodies>& out,uint64_t now) const;
    // The pushes since the last call, cleared.
    unsigned Touches(std::array<Touch,MaxTouches>& out);
    unsigned Simulated() const;
    void Reset();
};
}
