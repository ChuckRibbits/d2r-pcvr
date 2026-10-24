#pragma once
#include "skill_slots.h"
#include "strike_skills.h"
#include "toss.h"
#include "weapon_skills.h"

// How a skill works from a hand slot in VR, for the line under the skill
// picker and the hand wheel while one is pointed at (2026-10-23, the owner:
// "describe how each skill works, including when gesture assigned"). Where
// the slot and what the hand holds make it a gesture it says how: Smite with
// a shield, arrow skills with a bow, javelin and Double Throw throws, strike
// skills and thrown traps. Anything else casts natively, aimed by that hand.
// One or two short sentences: the line holds about 180 characters.
namespace framevr::skillusage {
inline constexpr size_t MaxLength=180;
inline constexpr int Interact=357;   // the catalog's Interact / pick up entry (native_ui)
inline const char* Describe(int skill,int slot) {
    namespace ws=weaponskills;namespace st=strikeskills;
    if(skill<0||slot<0||slot>=skills::Count)return "";
    const int hand=skills::Hand(slot);
    const bool trigger=slot==skills::TriggerSlot(hand),right=trigger&&hand==0,left=trigger&&hand==1;
    if(skill==Interact)return "Press to interact with or pick up what this hand points at.";
    if(skill==ws::Attack)return "Press to attack what this hand points at. Swinging a melee weapon attacks without it.";
    if(st::Known(skill)) {
        if(st::MultiHit(skill))
            return right?"Gesture: a full-cycle swing that lands casts it with its first hit; each later swing that lands delivers the next hit, with no more mana.":
                   left?"Gesture with a second weapon or claw: the left hand's landed swings cast it and deliver its later hits, as on the right trigger.":
                        "Press to cast it, aimed by this hand. On a trigger it is a gesture: landed swings cast it and deliver its hits.";
        return right?"Gesture: a full-cycle swing that lands casts it on what you hit; quicker swings stay basic attacks. Holding the trigger still casts it.":
               left?"Gesture with a second weapon or claw: a full-cycle swing of the left hand that lands casts it; quicker swings stay basic attacks.":
                    "Press to cast it, aimed by this hand. On a trigger it is a gesture: a full-cycle swing that lands casts it.";
    }
    if(skill==ws::Smite)
        return left?"Gesture with a shield: bash or thrust the shield into a monster to cast it. Without a shield, press to cast.":
                    "Press to cast it at what this hand points at. On the left trigger with a shield, a shield bash casts it.";
    if(ws::Bow(skill)) {
        if(trigger)return "Gesture with a bow: draw the string and release to fire it. Without a bow, press to cast.";
        return ws::QuiverSkill(skill)?"With a bow: loads it into every following arrow; press again for basic arrows. Without a bow, press to cast.":
                                      "Press to cast it natively. On a trigger with a bow, draw the string and release to fire it.";
    }
    if(ws::JavelinSkill(skill))
        return right?"Gesture with a javelin: arms your throw; throw by hand and let go to cast it. Without a javelin, press to cast.":
               left?"Press to cast it, aimed by this hand. With a javelin, the right trigger or a face button carries it on your throws.":
                    "With a javelin: loads it into every following throw; press again for basic throws. Without one, press to cast.";
    if(skill==ws::DoubleThrow)
        return right?"Gesture with two throwing weapons: each hand throws its own, the right with this trigger and the left with the left trigger.":
               left?"Press to cast it natively. On the right trigger with two throwing weapons, each hand throws its own.":
                    "With two throwing weapons: loads it, so each hand throws its own weapon; press again for basic throws.";
    if(toss::Skill(skill))
        return right?"Gesture: press to hold the trap or bomb, throw it and let go; it lands where the arc meets the floor. A quick press casts where you point.":
                     "Press to cast it, aimed by this hand. On the right trigger you can throw it to place it.";
    return trigger?"Press the trigger to cast it, aimed by this hand.":"Press the button to cast it, aimed by this hand.";
}
// A gesture from this slot with the right equipment (the description starts so).
inline bool Gesture(int skill,int slot) {
    const char* text=Describe(skill,slot);
    return text[0]=='G'&&text[1]=='e'&&text[2]=='s';
}
}
