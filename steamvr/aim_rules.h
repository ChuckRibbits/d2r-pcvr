#pragma once
#include <cstdint>

// How each skill uses the aimed monster, and which hand's target it takes
// (Jeff, 2026-10-20; the vault's Findings/Skill targeting map). One table, so
// an exception is a row here, not another path in the casts.
namespace framevr::aimrules {
enum class Use {
    Native,   // the game picks (self skills, Strafe, unit skills until they get the monster)
    Heading,  // projectiles and fans: toward the target, else the free aim
    Ground,   // a ground skill: on the target, else the free-aim floor; a landing circle
    Curse,    // centred on the target, else the free-aim floor; instant, no circle
    Attack,   // Charge, Leap Attack, Dragon Flight, Whirlwind: the target, else the free-aim spot
    Spot,     // Teleport, Leap, golems, Raise Skeleton: the free-aim spot, never the target
    CorpseAtTarget,   // Corpse Explosion, Poison Explosion: at the target, else the free-aim spot
};
// directed: the game's own "aimed" flag for the skill (spellaim::Directed).
inline Use Of(int skill,bool directed) {
    switch(skill) {
        // ground skills
        case 251: case 256: case 261: case 262: case 271: case 272: case 276: case 257: case 273:   // Assassin
        case 51: case 56: case 59: case 62:                                                         // Sorceress
        case 234: case 244:                                                                         // Druid
        case 78: case 88:                                                                           // Bone Wall, Bone Prison
            return Use::Ground;
        // curses and other areas on a monster
        case 66: case 71: case 72: case 76: case 77: case 81: case 82: case 86: case 87: case 91:  // Necromancer
        case 17:    // Slow Missiles
        case 137:   // Taunt
            return Use::Curse;
        // attacks that travel
        case 107: case 143: case 275: case 151:   // Charge, Leap Attack, Dragon Flight, Whirlwind
            return Use::Attack;
        // the pointed spot only
        case 54: case 132:                        // Teleport, Leap
        case 28:                                  // Decoy
        case 75: case 85: case 90: case 94:       // golems
        case 70: case 80: case 95:                // Raise Skeleton, Raise Skeletal Mage, Revive
        case 131: case 142: case 150:             // Find Potion, Find Item, Grim Ward
            return Use::Spot;
        case 74: case 83:                         // Corpse Explosion, Poison Explosion
            return Use::CorpseAtTarget;
        case 26:                                  // Strafe picks its own targets
            return Use::Native;
        default:
            return directed?Use::Heading:Use::Native;
    }
}
// The cast is placed at a point (on the floor), not sent along a heading.
inline bool Placed(Use use){return use==Use::Ground||use==Use::Curse||use==Use::Attack||use==Use::Spot||use==Use::CorpseAtTarget;}
// The aimed monster counts for it.
inline bool TakesTarget(Use use){return use==Use::Heading||use==Use::Ground||use==Use::Curse||use==Use::Attack||use==Use::CorpseAtTarget;}
// A landing circle shows where it will hit.
inline bool Circle(Use use){return use==Use::Ground;}
// Cells short of the target, toward the caster: Bone Wall rises in front of the
// monster, Bone Prison (and every other skill) on it (Jeff, 2026-10-20).
inline float FrontOfTarget(int skill){return skill==78?3.f:0.f;}
// Skills the game should be handed the monster itself, when there is one: seekers
// home in on it (Guided Arrow, Bone Spirit), Fist of the Heavens strikes it,
// Attract and Taunt need a unit, and the travelling attacks (Charge, Leap
// Attack, Dragon Flight) and Zeal fight it. With no monster they keep their rule.
inline bool WantsUnit(int skill){
    switch(skill){case 22: case 93: case 121: case 86: case 137: case 106: case 107: case 143: case 275: return true;default: return false;}
}

// Where the target comes from: the right hand (0), the left hand (1), the eyes (2).
enum Source {Right=0,Left=1,Eyes=2};
// The order to try: the button's own hand, the other hand, the eyes. A bow skill
// counts as the bow hand (left) while the bow points at something.
struct Order {Source at[3];};
inline Order For(int buttonHand,bool bowSkill) {
    if(bowSkill||buttonHand==1)return {{Left,Right,Eyes}};
    return {{Right,Left,Eyes}};
}
}
