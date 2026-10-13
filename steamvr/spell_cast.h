#pragma once
#include <algorithm>
#include <cstdint>

namespace framevr::spellcast {
// First audited family: straight, single-missile Sorceress casts. Other
// classes, homing missiles, fans, channels and ground skills use native input.
inline bool Supported(int id) {return id==36||id==39||id==45||id==47;}
inline bool Candidate(unsigned character,int id,int start,int action,int missile,unsigned flags) {
    return character==1&&Supported(id)&&start==0&&action==0&&missile>1&&missile<4096&&!(flags&3u);
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
}
