#pragma once
#include <algorithm>
#include <cstdint>
#include "frame_math.h"

namespace framevr::spellcast {
// Preserve continuous yaw, without amplifying tracking noise near vertical.
// No usable heading on the first sample means no shot, not a default compass
// direction. Each hand keeps its own fallback until the world/turn changes.
class PlanarAim {
    uint64_t session=0,epoch=0;
    math::Vec last{};
    static bool Project(math::Vec in,math::Vec& out) {
        if(!std::isfinite(in.x)||!std::isfinite(in.y)||!std::isfinite(in.z))return false;
        const float horizontal=in.x*in.x+in.z*in.z,total=horizontal+in.y*in.y;
        if(total<.000001f||horizontal<total*.0076f)return false; // within five degrees of vertical
        out=math::Normalized({in.x,0,in.z});return true;
    }
public:
    math::Vec Update(uint64_t world,uint64_t turn,math::Vec direction,math::Vec fallback,bool planar=true) {
        if(world!=session||turn!=epoch){session=world;epoch=turn;last={};}
        if(!Project(direction,last)&&math::Dot(last,last)<.5f)Project(fallback,last);
        return planar?last:direction;
    }
};
inline bool NativePath(int id) {return id==38;} // Charged Bolt keeps its native wandering path and grid hits.
inline bool Supported(int id) {return id==36||id==38||id==39||id==45||id==47;}
inline unsigned LaunchCapacity(int id) {return NativePath(id)?24:1;}
inline bool Candidate(unsigned character,int id,int start,int action,int missile,unsigned flags,int secondary=-1) {
    if(character!=1||!Supported(id)||start!=0||(flags&3u))return false;
    if(NativePath(id))return action==17&&missile==-1&&secondary==56;
    return action==0&&missile>1&&missile<4096;
}
// Ordinary Sorceress cast animation: 14 frames, 256 base animation speed,
// diminishing returns capped at 75 effective FCR, minus the terminal frame.
inline unsigned CastFrames(int fcr) {
    fcr=std::clamp(fcr,0,100000);
    const int effective=std::min(75,120*fcr/(120+fcr));
    const int speed=256*(100+effective)/100;
    return unsigned((14*256+speed-1)/speed-1);
}
class Cadence {
    uint64_t session=0;
    uint32_t last[2]{};
    unsigned duration[2]{};
    bool used[2]{};
public:
    bool Ready(uint64_t world,int hand,uint32_t tick) const {
        return world&&hand>=0&&hand<2&&(world!=session||!used[hand]||uint32_t(tick-last[hand])>=duration[hand]);
    }
    void Fired(uint64_t world,int hand,uint32_t tick,unsigned frames) {
        if(world!=session){session=world;used[0]=used[1]=false;}
        if(hand<0||hand>1)return;
        used[hand]=true;last[hand]=tick;duration[hand]=std::clamp(frames,1u,250u);
    }
};
// One short-lived notification shared by both hands. The caller synchronizes
// game/UI access. Native speech supplies the longer voice repeat cooldown;
// this only coalesces held-trigger failures and prevents stale delivery.
class ManaWarning {
    uint64_t session=0,at=0;
    uint32_t player=0;
    bool pending=false,issued=false;
public:
    void Request(uint64_t world,uint32_t id,uint64_t now) {
        if(!world)return;
        if(world==session&&id==player&&issued&&now>=at&&now-at<250)return;
        session=world;player=id;at=now;pending=issued=true;
    }
    bool Take(uint64_t world,uint32_t id,uint64_t now,bool active) {
        const bool take=pending&&active&&world&&world==session&&id==player&&now>=at&&now-at<=500;
        pending=false;
        return take;
    }
};
}
