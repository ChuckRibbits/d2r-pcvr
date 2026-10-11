#pragma once
#include "body_tracking.h"
#include <cstdint>
namespace framevr::shoulder {
struct Input {
    double time=0;
    uint64_t session=0,epoch=0;
    bool active=false,tracked=false,allowed=false;
    int dominant=0;
    math::Vec head{},hand{};math::Quat heading{0,0,0,1};
    float trigger=0;
};
struct Output {bool swap=false,block=false,inReach=false;};
inline bool Near(math::Vec hand,math::Vec head,math::Quat heading){
    if(!body::Finite(hand)||!body::Finite(head))return false;
    const auto p=math::Rotate(math::Conjugate(heading),hand-head);
    // Either shoulder, behind the head's vertical plane. Face-level/front
    // gestures and hands at the hips are deliberately outside this volume.
    return std::fabs(p.x)>=.10f&&std::fabs(p.x)<=.48f&&p.y>=-.38f&&p.y<=.16f&&p.z>=.025f&&p.z<=.38f;
}
class Gesture {
    bool primed=false,triggerWas=false,consumed=true;
    double previous=0,nearSince=0;
    uint64_t session=0,epoch=0;
    int dominant=-1;
public:
    void Reset(){*this=Gesture{};}
    Output Step(const Input& in){
        Output out;
        const bool down=std::isfinite(in.trigger)&&in.trigger>(triggerWas?.20f:.55f);
        const bool valid=in.active&&in.tracked&&in.session&&std::isfinite(in.time)&&in.time>0&&
            body::Finite(in.head)&&body::Finite(in.hand);
        if(!valid||!primed||session!=in.session||epoch!=in.epoch||dominant!=in.dominant||in.time<=previous||in.time-previous>.15){
            Reset();primed=valid;session=in.session;epoch=in.epoch;dominant=in.dominant;previous=in.time;
            triggerWas=down;consumed=down;out.block=down;return out;
        }
        previous=in.time;const bool edge=down&&!triggerWas;triggerWas=down;
        if(!down)consumed=false;
        out.inReach=Near(in.hand,in.head,in.heading);
        if(!out.inReach)nearSince=0;else if(!nearSince)nearSince=in.time;
        out.block=consumed||(out.inReach&&in.allowed);
        if(out.inReach&&in.allowed&&edge&&in.time-nearSince>=.12){consumed=true;out.swap=out.block=true;}
        return out;
    }
};
}
