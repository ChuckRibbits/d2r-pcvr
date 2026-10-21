#include "companions.h"
#include "interaction_paint.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace framevr;
int main() {
    constexpr uintptr_t base=0x10000,player=base,data=base+0x100,pets=base+0x200,lists=base+0x300;
    std::vector<uint8_t> memory(0x6000);
    auto put=[&](uintptr_t p,const auto& value){assert(p>=base&&p-base+sizeof value<=memory.size());std::memcpy(memory.data()+p-base,&value,sizeof value);};
    auto read=[&](uintptr_t p,auto& value){if(p<base||p-base>memory.size()||sizeof value>memory.size()-(p-base))return false;std::memcpy(&value,memory.data()+p-base,sizeof value);return true;};
    uint32_t header[4]={0,2,42,1};put(player,header);put(player+0x10,data);put(data+0x98,pets);put(pets,lists);
    auto sample=[&]{return companions::ReadRoster(player,22,read);};
    auto empty=sample();assert(empty.known&&companions::Count(empty)==0);
    auto list=[&](int type,uintptr_t head,int current,int maximum) {
        put(lists+type*32,head);put(lists+type*32+8,int32_t(current));put(lists+type*32+12,int32_t(maximum));
    };
    auto node=[&](uintptr_t p,uint32_t id,uintptr_t next,uint32_t flags=0){put(p,flags);put(p+4,id);put(p+24,next);};
    const auto a=base+0x1000,b=a+32,c=b+32;
    node(a,101,b);node(b,102,0);list(4,a,2,8);
    auto two=sample();assert(two.known&&two.alive[4]==2&&companions::Count(two)==1);
    // Native removal decreases the roster even when no view/position data exists.
    node(a,101,0);list(4,a,1,8);assert(sample().alive[4]==1);
    list(4,0,0,8);auto zero=sample();assert(zero.known&&zero.alive[4]==0&&(zero.relevant&companions::Bit(4)));
    // Hireling's default capacity alone is not an owned companion. Its retained
    // dead entry is visible as zero, and resurrection restores one.
    list(7,0,0,1);assert(!(sample().relevant&companions::Bit(7)));
    node(c,103,0,1);list(7,c,1,1);assert(sample().alive[7]==0&&(sample().relevant&companions::Bit(7)));
    node(c,103,0);assert(sample().alive[7]==1);
    list(4,a,1,8);node(a,101,a);assert(!sample().known); // cycle
    node(a,101,0);list(4,a,2,8);assert(!sample().known); // incomplete list
    list(4,a,1,8);list(5,a,1,8);assert(!sample().known); // duplicate across groups
    list(5,0,0,0);list(4,a,-1,8);assert(!sample().known);
    list(4,a,513,8);assert(!sample().known);
    list(4,UINTPTR_MAX-8,1,8);assert(!sample().known);
    list(4,0,0,8);put(data+0x98,uintptr_t(0));assert(sample().known);put(data+0x98,pets);
    for(unsigned types:{0u,19u,65u})assert(!companions::ReadRoster(player,types,read).known);
    for(uint32_t mode:{0u,17u}){header[3]=mode;put(player,header);assert(!sample().known);}
    header[3]=1;header[0]=1;put(player,header);assert(!sample().known);header[0]=0;put(player,header);
    assert(!companions::ReadRoster(player,22,[](uintptr_t,auto&){return false;}).known);
    assert(companions::SkillType(70)==4&&companions::SkillType(94)==3&&companions::SkillType(237)==12);
    assert(companions::SkillType(279)==16&&companions::SkillType(382)==21&&companions::SkillType(74)==0);
    for(int type:{4,5,3,6})zero.relevant|=companions::Bit(type);
    assert(companions::Count(zero)==4&&companions::Rows(zero)==1);
    for(const auto& d:companions::Definitions)zero.relevant|=companions::Bit(d.type);
    assert(companions::Count(zero)==19&&companions::UsedHeight(zero)==companions::Height);
    interaction::Snapshot s;s.session=1;s.at=200;s.companions=two;
    auto painted=interaction::Context(s,{},true);
    assert(interaction::CompanionImageCurrent(painted,s));
    s.companions.alive[4]=1;assert(!interaction::CompanionImageCurrent(painted,s));
    s.companions=two;s.session=2;assert(!interaction::CompanionImageCurrent(painted,s));
    s.session=1;s.companions.known=false;assert(!interaction::CompanionImageCurrent(painted,s));
    s.companions=two;painted.fresh=false;assert(!interaction::CompanionImageCurrent(painted,s));
    for(bool palm:{false,true})for(bool vitals:{false,true})for(int rows:{1,2,3}) {
        interaction::Pose hand{{.1f,.9f,-.4f},math::Yaw(.3f)},head{{0,1.6f,0},{0,0,0,1}};
        const auto map=interaction::NavigationMapPanel(hand,head,palm,vitals,interaction::CompanionSpace(rows));
        const auto footer=interaction::NavigationCompanionsPanel(hand,head,palm,vitals,rows);
        const auto delta=math::Rotate(math::Conjugate(map.q),footer.p-map.p);
        assert(std::fabs(delta.x)<.0001f&&std::fabs(delta.z)<.0001f);
        assert(std::fabs(delta.y+(interaction::WristMapSize+interaction::CompanionHeight(rows))*.5f+.006f)<.0001f);
        if(vitals) {
            const auto panel=interaction::VitalsPanel(hand,head,palm);
            const auto distance=math::Rotate(math::Conjugate(panel.q),footer.p-panel.p).y;
            assert(distance-interaction::CompanionHeight(rows)*.5f>(palm?interaction::PalmHeight:interaction::WristHeight)*.5f+.009f);
        }
    }
    std::puts("Companions: owned roster counts, deaths, zero groups, bounds, stale images and wrist/palm spacing passed");
}
