// Keep native crossbow attacks and paths; replace only their launch placement.
#include <windows.h>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <cstring>
#include <cstdio>
#include "crossbow.h"
#include "skeletons.h"
#include "combat.h"
#include "memory.h"
#include "logging.h"
#include "../shared/d2r_vr_state.h"
namespace gamestate {uint64_t LocalPlayer();uint32_t HandsKey();uint32_t WeaponSet();uint32_t WeaponType();bool MenuOpen();}
namespace framevr {uint64_t BowRenderTime();}
namespace crossbowvr {
namespace {
using namespace framevr::crossbow;
std::mutex mutex;
struct GateState {bool active=false;uint64_t session=0,at=0;uint32_t equipment=0,epoch=0;} gate;
struct Bolt {
    uintptr_t unit=0;uint32_t id=0,entity=UINT32_MAX;Launch launch{};
    Mat rendered{},previous{};uint64_t renderAt=0;
};
std::array<Bolt,128> bolts{};
std::unordered_map<uint32_t,size_t> bindings;
std::atomic<unsigned> allocated{0},visible{0},prepared[2]{},bound{0},painted{0};
struct Pending {uint32_t owner=0;int tx=0,ty=0;Launch shot{};};
std::array<Pending,16> pending{};size_t nextPending=0;
// Native creation can initialize the render entity before returning its unit.
thread_local std::array<const Launch*,8> creating{};
thread_local unsigned depth=0;
template<class T> bool Read(uintptr_t p,T& v){return p>0x10000&&framevr::ReadMemory(&v,reinterpret_cast<void*>(p),sizeof v);}
bool Header(uintptr_t unit,uint32_t (&h)[3]){return Read(unit,h)&&h[0]==3&&h[1]==31;}
void Clear(Bolt& b) {
    if(!b.unit)return;
    const auto it=bindings.find(b.entity);
    if(it!=bindings.end()&&it->second==size_t(&b-bolts.data()))bindings.erase(it);
    b={};--allocated;visible=unsigned(bindings.size());
}
void Put(std::array<uint8_t,0x98>& p,size_t at,int value){std::memcpy(p.data()+at,&value,sizeof value);}
}
void Gate(bool active,uint64_t session,uint32_t equipment,uint32_t epoch) {
    std::lock_guard lock(mutex);
    if(gate.session!=session){bolts={};pending={};bindings.clear();allocated=visible=0;}
    gate={active,session,GetTickCount64(),equipment,epoch};
}
bool Prepare(uintptr_t params,std::array<uint8_t,0x98>& adjusted,Launch& shot,bool server) {
    uint32_t missile=0,ownerHeader[3]{},id=0;uintptr_t owner=0;int skill=-1;
    if(gamestate::WeaponType()!=D2RVR_TYPE_CROSSBOW||gamestate::MenuOpen()||
       !Read(params+0x20,missile)||missile!=31||!Read(params+0x3c,skill)||skill!=0||
       !Read(params+8,owner)||!Read(owner,ownerHeader)||ownerHeader[0]!=0||
       !combat::LocalPlayerId(id)||ownerHeader[2]!=id)return false;
    const auto now=GetTickCount64();GateState g;Pose pose;bool paired=false;
    int target[2]{};if(!Read(params+0x2c,target)||!Read(params,adjusted))return false;
    {
        std::lock_guard lock(mutex);g=gate;
        if(!server)for(auto& p:pending)if(p.shot.pose.at&&p.owner==id&&
           ((p.tx==target[0]&&p.ty==target[1])||(p.shot.tx==target[0]&&p.shot.ty==target[1]))&&
           Fresh(p.shot.pose,now,g.session,g.equipment,g.epoch)) {
            shot=p.shot;p={};paired=true;break;
        }
    }
    if(!g.active||now<g.at||now-g.at>100||g.session!=gamestate::LocalPlayer()||
       g.equipment!=(gamestate::HandsKey()^(gamestate::WeaponSet()<<30)))return false;
    if(!paired&&(!skel::CrossbowPose(pose)||!Fresh(pose,now,g.session,g.equipment,g.epoch)||!Aim(pose,shot)))return false;
    if(server){std::lock_guard lock(mutex);pending[nextPending++%pending.size()]={id,target[0],target[1],shot};}
    uint32_t flags=0;std::memcpy(&flags,adjusted.data(),sizeof flags);flags|=1; // explicit native start coordinates
    std::memcpy(adjusted.data(),&flags,sizeof flags);
    Put(adjusted,0x24,shot.x);Put(adjusted,0x28,shot.y);Put(adjusted,0x2c,shot.tx);Put(adjusted,0x30,shot.ty);
    ++prepared[server?1:0];return true;
}
void BeginClient(const Launch* shot){if(depth<creating.size())creating[depth]=shot;++depth;}
void EndClient(){if(depth)--depth;}
void Created(uintptr_t unit,const Launch& shot) {
    uint32_t h[3]{};if(!Header(unit,h))return;
    std::lock_guard lock(mutex);
    if(shot.pose.session!=gate.session)return;
    Bolt* vacant=nullptr;
    for(auto& b:bolts) {
        if(b.unit==unit&&b.id==h[2])return;
        if(b.unit==unit)Clear(b); // allocator reuse cannot inherit an old entity
        if(!b.unit&&!vacant)vacant=&b;
    }
    if(!vacant)for(auto& b:bolts) {
        uint32_t current[3]{};
        if(!Header(b.unit,current)||current[2]!=b.id){Clear(b);if(!vacant)vacant=&b;}
    }
    if(vacant){*vacant={unit,h[2],UINT32_MAX,shot};++allocated;}
}
void Destroyed(uintptr_t unit) {
    if(!allocated.load(std::memory_order_relaxed))return;
    std::lock_guard lock(mutex);for(auto& b:bolts)if(b.unit==unit)Clear(b);
}
bool Visual(uint32_t entity,uintptr_t unit,bool releasePoint) {
    if(!releasePoint)return false;
    if(depth&&depth<=creating.size()&&creating[depth-1])Created(unit,*creating[depth-1]);
    uint32_t h[3]{};if(!Header(unit,h))return false;
    std::lock_guard lock(mutex);
    for(auto& b:bolts)if(b.unit==unit&&b.id==h[2]&&b.launch.pose.session==gate.session) {
        if(b.entity!=UINT32_MAX&&b.entity!=entity)return false;
        if(b.entity==UINT32_MAX)++bound;
        b.entity=entity;bindings[entity]=size_t(&b-bolts.data());visible=unsigned(bindings.size());return true;
    }
    return false;
}
void Transform(uintptr_t out,uint32_t entity,uintptr_t component) {
    if(!visible.load(std::memory_order_relaxed))return;
    std::lock_guard lock(mutex);const auto it=bindings.find(entity);
    if(it==bindings.end())return;
    auto& bolt=bolts[it->second];
    if(bolt.launch.pose.session!=gate.session)return;
    uint32_t h[3]{},position[2]{};uintptr_t path=0;
    if(!Header(bolt.unit,h)||h[2]!=bolt.id){Clear(bolt);return;}
    const auto at=framevr::BowRenderTime();
    if(bolt.renderAt!=at||bolt.rendered.m[15]!=1) {
        if(!Read(bolt.unit+0x38,path)||!Read(path,position))return;
        const Vec grid={position[0]/32768.f,0,position[1]/32768.f};Mat matrix;
        if(!Matrix(bolt.launch,grid,matrix))return;
        bolt.previous=bolt.rendered.m[15]==1?bolt.rendered:matrix;
        bolt.rendered=matrix;bolt.renderAt=at;
    }
    ++painted;
    framevr::WriteMemory(reinterpret_cast<void*>(out),&bolt.rendered,sizeof(Mat));
    framevr::WriteMemory(reinterpret_cast<void*>(component+0x40),&bolt.rendered,sizeof(Mat));
    framevr::WriteMemory(reinterpret_cast<void*>(component+0x80),&bolt.previous,sizeof(Mat));
}
void Report() {
    const auto c=prepared[0].exchange(0),s=prepared[1].exchange(0),b=bound.exchange(0),p=painted.exchange(0);
    if(!c&&!s&&!b&&!p)return;
    char message[180];std::snprintf(message,sizeof message,"Frame crossbow: muzzle launches client/server %u/%u, release meshes %u, transforms %u",c,s,b,p);
    framevr::logging::Write(message);
}

}
