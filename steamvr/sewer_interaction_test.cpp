#include "sewer_interaction.h"
#include "interact_press.h"
#include "sewer_prompt.h"
#include <cassert>
#include <cstring>
#include <limits>
#include <map>
#include <vector>
using namespace framevr::sewerinteraction;
namespace {
struct Memory {
    std::map<uintptr_t,std::vector<uint8_t>> blocks;
    template<class T>void Put(uintptr_t p,const T& value) {
        auto& b=blocks[p];b.resize(sizeof value);std::memcpy(b.data(),&value,sizeof value);
    }
    template<class T>bool Read(uintptr_t p,T& value) {
        auto i=blocks.find(p);if(i==blocks.end()||i->second.size()!=sizeof value)return false;
        std::memcpy(&value,i->second.data(),sizeof value);return true;
    }
};
constexpr uintptr_t Player=0x100000,Path=0x110000,Room=0x120000,Near=0x130000,OtherRoom=0x140000;
constexpr uintptr_t Pot=0x200000,Exit=0x210000,ExitPath=0x310000,OtherExit=0x220000;
Memory Fixture() {
    Memory m;const uint32_t h[]={0,3,1,1},xy[]={100*65536u,100*65536u};
    m.Put(Player,h);m.Put(Player+0x38,Path);m.Put(Path,xy);m.Put(Path+0x20,Room);
    m.Put(Room,Near);m.Put(Room+0x40,2u);m.Put(Near,Room);m.Put(Near+8,OtherRoom);
    m.Put(Room+0xa8,Pot);m.Put(OtherRoom+0xa8,uintptr_t(0));
    const uint32_t pot[]={2,5,10},exit[]={5,57,20},position[]={103,100};
    m.Put(Pot,pot);m.Put(Pot+0x160,Exit);m.Put(Pot+0x158,uintptr_t(0));
    m.Put(Exit,exit);m.Put(Exit+0x160,uintptr_t(0));m.Put(Exit+0x38,ExitPath);
    m.Put(ExitPath,Room);m.Put(ExitPath+0x10,position);
    return m;
}
Result Query(Memory& m,int level=80,float x=1,float y=0,bool allow=true) {
    return Find(Player,level,x,y,[&](uintptr_t p,auto& v){return m.Read(p,v);},
                [&](uintptr_t player,uintptr_t unit){assert(player==Player);assert(unit==Exit||unit==OtherExit);return allow;});
}
}
int main() {
    auto m=Fixture();
    auto result=Query(m);assert(result.unit==Exit&&result.id==20&&result.distance==3);
    assert(result.visited==2&&result.warps==1&&result.facing==1);
    // The room link must find the exit behind a non-warp even when its hash
    // link is null. Both Kurast regions work; other levels and tiles do not.
    assert(Query(m,81).unit==Exit);
    for(int level:{0,79,82,92,93})assert(!Query(m,level).unit);
    for(uint32_t type:{0u,1u,2u,4u}) {
        auto bad=m;const uint32_t h[]={type,57,20};bad.Put(Exit,h);assert(!Query(bad).unit);
    }
    for(uint32_t warp:{56u,58u,59u}) {
        auto bad=m;const uint32_t h[]={5,warp,20};bad.Put(Exit,h);assert(!Query(bad).unit);
    }
    assert(!Query(m,80,-1,0).unit); // Behind the player.
    assert(!Query(m,80,0,1).unit); // Beside the player.
    assert(!Query(m,80,0,0).unit);
    assert(!Query(m,80,std::numeric_limits<float>::quiet_NaN(),0).unit);
    assert(!Query(m,80,1,0,false).unit); // Native range/line check refused.
    auto bad=m;const uint32_t far[]={109,100};bad.Put(ExitPath+0x10,far);assert(!Query(bad).unit);
    bad=m;bad.Put(ExitPath,OtherRoom);assert(!Query(bad).unit); // Stale/wrong owner room.
    for(uint32_t mode:{0u,5u,6u,17u}) {
        bad=m;const uint32_t h[]={0,3,1,mode};bad.Put(Player,h);assert(!Query(bad).unit);
    }
    bad=m;bad.Put(Room+0x40,65u);assert(!Query(bad).unit);
    bad=m;bad.blocks.erase(Exit+0x38);assert(!Query(bad).unit);
    bad=m;bad.Put(Pot+0x160,Pot);assert(!Query(bad).unit); // Corrupt cycle is bounded.
    // A nearer exit that fails the native check cannot hide a usable farther one.
    m.Put(OtherRoom+0xa8,OtherExit);const uint32_t h[]={5,57,21},pos[]={101,100};
    constexpr uintptr_t OtherPath=0x320000;
    m.Put(OtherExit,h);m.Put(OtherExit+0x160,uintptr_t(0));m.Put(OtherExit+0x38,OtherPath);
    m.Put(OtherPath,OtherRoom);m.Put(OtherPath+0x10,pos);
    assert(Query(m).unit==OtherExit);
    result=Find(Player,80,1,0,[&](uintptr_t p,auto& v){return m.Read(p,v);},
                [](uintptr_t,uintptr_t unit){return unit==Exit;});
    assert(result.unit==Exit);
    // Keep one-target-per-press semantics when entering changes the target.
    framevr::interaction::InteractPress press;
    assert(press.Poll(true,Exit,true,true,1,0).down);
    assert(!press.Poll(true,OtherExit,true,true,1,0).down);
    press.Poll(false,OtherExit,true,true,1,0);
    assert(press.Poll(true,OtherExit,true,true,1,0).down);
    // Prompt lifetime is independent of the native unit pointer: travel,
    // loading, stale updates and session changes must hide the instruction.
    namespace prompt=framevr::sewerprompt;
    prompt::Target t{1,1000,80,20,103,100};
    assert(prompt::Fresh(t,1,1200,80));
    assert(!prompt::Fresh(t,1,1201,80));
    assert(!prompt::Fresh(t,1,999,80));
    assert(!prompt::Fresh(t,2,1000,80));
    assert(!prompt::Fresh(t,1,1000,92));
    assert(!prompt::Fresh({},1,1000,80));
    auto invalid=t;invalid.x=std::numeric_limits<float>::quiet_NaN();
    assert(!prompt::Fresh(invalid,1,1000,80));
    // Elevated ground has shear on BOTH horizontal axes. A quarter-turn must
    // rotate the entrance into room coordinates, including after a snap turn.
    namespace math=framevr::math;
    namespace coordinates=framevr::coordinates;
    const auto ground=coordinates::RenderGround({50,0,50},2);
    const math::Vec hero=ground*4,origin{1,1.6f,2},head{1,1.6f,2};
    math::Vec at{};
    assert(prompt::Position(t,origin,hero,{0,0,0,1},4,6.4f,head,{0,0,0,1},at));
    assert(std::abs(at.x-2.5f)<.001f&&std::abs(at.z-2)<.001f&&std::abs(at.y-1.38f)<.001f);
    assert(prompt::Position(t,origin,hero,math::Yaw(1.57079633f),4,6.4f,head,{0,0,0,1},at));
    assert(std::abs(at.x-1)<.001f&&std::abs(at.z-3.5f)<.001f);
    auto close=t;close.x=100;close.y=100;
    assert(prompt::Position(close,origin,hero,{0,0,0,1},4,6.4f,head,{0,0,0,1},at));
    assert(std::abs(at.z-1.1f)<.001f); // Readable even directly on the trigger.
    assert(!prompt::Position(t,origin,hero,{0,0,0,1},0,6.4f,head,{0,0,0,1},at));
    assert(!prompt::Position(t,origin,hero,{0,0,0,0},4,6.4f,head,{0,0,0,1},at));
}
