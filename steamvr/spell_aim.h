#pragma once
#include "frame_math.h"
#include "world_coordinates.h"
#include <algorithm>
#include <cstdint>
#include <cstddef>

namespace framevr::spellaim {
struct Cast {bool directed=false;float x=0,y=0;bool contactPoint=false;};
struct Scope {
    Cast& current;Cast previous;
    Scope(Cast& current,Cast next):current(current),previous(current){current=next;}
    ~Scope(){current=previous;}
    Scope(const Scope&)=delete;
};
// Compiled Skills flags and server behavior, validated against the owner's
// local skills.bin/text tables. Class and equipped weapon do not decide which
// spells are directional. Target-only/corpse skills keep native selection.
inline bool Directed(uint32_t flags,int missile,int serverFunction) {
    if(flags&((1u<<4)|(1u<<20)|(1u<<24)))return false; // passive, targetable-only, corpse
    if(missile>=0)return true;
    switch(serverFunction) {
        case 8:  // Teeth / multi-shot / Shock Wave: preserve native fan
        case 10: // initially directed seeking missile
        case 17: // Charged Bolt
        case 19: // Inferno / Arctic Blast
        case 26: // Chain Lightning
        case 48: // Blade Fury
        case 118: // Twister / Tornado
            return true;
        default:return false; // ground placements, summons, self/area effects
    }
}
inline bool Horizontal(math::Vec direction,float& x,float& y) {
    const float n=std::hypot(direction.x,direction.z);
    if(!std::isfinite(direction.x+direction.y+direction.z)||n<.001f)return false;
    x=direction.x/n;y=direction.z/n;return true;
}
// Native skill-command validation rejects abs(target-player)>50 on EITHER
// grid axis (3.3.93787: 0x4fe076 -> 0x4fe1f0). A 60-cell ray only passes near
// diagonals, leaving stale client aim between accepted casts. Stay inside that
// square with margin for movement between client resolution and server use.
// This point encodes heading; it does not change native missile lifetime/range.
inline constexpr float AimDistance=40.f;
inline bool Point(float x,float y,math::Vec direction,float& targetX,float& targetY) {
    float dx=0,dy=0;
    if(!std::isfinite(x+y)||!Horizontal(direction,dx,dy))return false;
    targetX=x+dx*AimDistance;targetY=y+dy*AimDistance;return true;
}
// Skills whose missile explodes where its flight ends (missiles.txt
// AlwaysExplode): the aimed point is the landing point, so a fixed heading
// distance would always throw them to full range. Fire Blast ("Fire Trauma",
// srvmissile "bomb in air") is the only player skill this way in 3.3.
inline bool Lobbed(int skill) {return skill==251;}
// Other skills cast at a ground point (Skills.txt ids): traps and Shock Web,
// Sorceress Blizzard / Meteor / Fire Wall / Hydra, Druid Fissure / Volcano.
// They take the same point as a bomb: the locked monster, else the floor
// point aimed at (VR Settings > Aim > Ground skills land where you aim).
inline bool GroundTarget(int skill) {
    switch(skill) {
        case 251: case 256: case 261: case 262: case 271: case 272: case 276:  // Assassin
        case 51: case 56: case 59: case 62:                                    // Sorceress
        case 234: case 244:                                                    // Druid
            return true;
        default: return false;
    }
}
// The landing circle's colour by element: 0 fire, 1 cold, 2 lightning, 3 poison/other.
enum Element { Fire, Cold, Lightning, Other };
inline Element ElementOf(int skill) {
    switch(skill) {
        case 59: return Cold;
        case 256: case 261: case 271: case 276: return Lightning;
        case 251: case 262: case 272: case 51: case 56: case 62: case 234: case 244: return Fire;
        default: return Other;
    }
}
inline constexpr float GroundMinDistance=2.f;
// Land a lobbed skill where the aiming ray meets the player's floor. Render
// units are converted to native cells by cellsPerUnit after removing the
// floor shear (coordinates::GridPlane). Too short or too far points are kept
// on the ray's heading at the nearest allowed distance; a ray that never
// descends (HandRay always tilts it down) is refused.
inline bool GroundPoint(math::Vec origin,math::Vec direction,float floorY,float cellsPerUnit,
                        float playerX,float playerY,float& targetX,float& targetY) {
    if(!std::isfinite(origin.x+origin.y+origin.z+direction.x+direction.y+direction.z+floorY+cellsPerUnit+playerX+playerY)||
       cellsPerUnit<=0||direction.y>=-1e-4f||origin.y<=floorY)return false;
    const float t=(floorY-origin.y)/direction.y;
    const auto hit=coordinates::GridPlane({origin.x+direction.x*t,floorY,origin.z+direction.z*t});
    float dx=hit.x*cellsPerUnit-playerX,dy=hit.z*cellsPerUnit-playerY;
    const float distance=std::hypot(dx,dy);
    if(distance<.001f) {
        if(!Horizontal(direction,dx,dy))return false;
        targetX=playerX+dx*GroundMinDistance;targetY=playerY+dy*GroundMinDistance;return true;
    }
    const float scale=std::clamp(distance,GroundMinDistance,AimDistance)/distance;
    targetX=playerX+dx*scale;targetY=playerY+dy*scale;return true;
}
// The controller quick-cast path takes integer grid cells. Refuse malformed
// positions before conversion, rather than returning an unrelated old target.
inline bool ControllerPoint(float x,float y,math::Vec direction,int& targetX,int& targetY) {
    float tx=0,ty=0;
    if(!Point(x,y,direction,tx,ty)||tx<1||ty<1||tx>65534||ty>65534)return false;
    targetX=int(tx);targetY=int(ty);return true;
}
// Prefix of the private stack command assembled by native MapClick. The
// resolver's point is not authoritative: MapClick selects a unit again and
// may replace that point before checking/dispatching the skill command.
struct NativeCommand {
    uint32_t flags,padding;
    uintptr_t actor,target;
    int x,y;
};
static_assert(offsetof(NativeCommand,actor)==0x08&&offsetof(NativeCommand,target)==0x10&&
              offsetof(NativeCommand,x)==0x18&&sizeof(NativeCommand)==0x20);
struct CommandAim {uintptr_t actor=0;int x=0,y=0;};
inline bool ControllerCast(int type) {return type>=7&&type<=10;}
inline bool ApplyCommand(const CommandAim& aim,NativeCommand& command) {
    if(!aim.actor||command.actor!=aim.actor||aim.x<1||aim.y<1||aim.x>65534||aim.y>65534)return false;
    command.target=0;command.x=aim.x;command.y=aim.y;
    return true;
}
// Apply after native resolution, including paths that use movement/facing
// without calling AttackPoint. Non-directional skills retain native targets.
inline bool Resolve(const Cast& cast,float playerX,float playerY,float& targetX,float& targetY) {
    if(!cast.directed)return false;
    if(cast.contactPoint) {
        if(!std::isfinite(cast.x)||!std::isfinite(cast.y))return false;
        targetX=cast.x;targetY=cast.y;return true;
    }
    return Point(playerX,playerY,{cast.x,0,cast.y},targetX,targetY);
}
}
