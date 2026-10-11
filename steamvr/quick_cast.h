#pragma once
#include <cstdint>
#include <cmath>
#include "skill_slots.h"
namespace framevr::skills {
// Two phases: claim face buttons before menu shortcuts, then resolve a cast
// after the current tracking, interaction ownership and native snapshot checks.
class QuickCast {
    uint8_t faces=0,previous=0,fresh=0,consumed=0;
    bool layer=false,alternate=false,primed=false;
    int selected=-1;
    uint64_t session=0,epoch=0;
    uint32_t equipment=0;
    int bank=-1;
    double at=0;
public:
    void Suspend(){selected=-1;layer=false;primed=false;consumed|=faces;}
    bool Capture(bool allowed,float trigger,uint8_t buttons) {
        faces=buttons&15;fresh=faces&~previous;previous=faces;
        consumed&=faces;
        layer=allowed&&std::isfinite(trigger)&&trigger>(layer?.2f:.55f);
        if(layer)consumed|=faces;
        else selected=-1;
        return layer;
    }
    bool Suppress(int face)const{return layer||(consumed&(1u<<face));}
    bool Layer()const{return layer;}
    int Resolve(bool allowed,bool second,uint8_t available,uint64_t currentSession,
                uint64_t currentEpoch,uint32_t currentEquipment,int currentBank,double now) {
        const bool changed=!primed||session!=currentSession||epoch!=currentEpoch||
            equipment!=currentEquipment||bank!=currentBank||!std::isfinite(now)||now<=at||now-at>.1;
        primed=std::isfinite(now);session=currentSession;epoch=currentEpoch;
        equipment=currentEquipment;bank=currentBank;at=now;
        if(!allowed||!layer||changed||!session) {selected=-1;alternate=second;return -1;}
        // One held shortcut at a time. Changing bank mid-hold must not cast a
        // different ability; release and press again to choose the new slot.
        if(!faces||(faces&(faces-1))||second!=alternate)selected=-1;
        alternate=second;
        if(faces&&!(faces&(faces-1))&&(fresh&faces)) {
            for(int i=0;i<4;++i)if(faces==(1u<<i))selected=i+(second?4:0);
        }
        if(selected>=0&&(!(faces&(1u<<(selected%4)))||!(available&(1u<<selected))))selected=-1;
        return selected;
    }
};
}
