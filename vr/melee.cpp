// Free-swing contact -> native basic attack, on the authoritative game thread.
// All addresses/signatures are for D2R 3.3.93787. No HP/stat/table/save edits.
#include <windows.h>
#include <intrin.h>
#include <D2RLPlugin/api.h>
#include <atomic>
#include <mutex>
#include <array>
#include <cstdio>
#include "melee.h"
#include "combat.h"
#include "sigscan.h"
#include "memory.h"
#include "logging.h"
#include "../steamvr/transport.h"
namespace gamestate {uint64_t LocalPlayer();bool MenuOpen();uint32_t HandsKey();uint32_t WeaponSet();}
namespace melee {
namespace {
using namespace framevr::melee;
constexpr uint64_t AttackRva=0x42feb0,TargetRva=0x48fe20,LookupRva=0x48fe80,RangeRva=0x348650;
constexpr uint64_t FlagsRva=0x44b720,RelationRva=0x48e460,LineRva=0x3502a0,StatsRva=0x975b0;
constexpr uint8_t AttackSig[]={0x40,0x55,0x56,0x41,0x54,0x41,0x56,0x48,0x8d,0xac,0x24,0xe8,0xfe,0xff,0xff,0x48,0x81,0xec,0x18,0x02,0x00,0x00};
constexpr uint8_t TargetSig[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xda,0xe8,0x22,0x1d,0x00,0x00};
constexpr uint8_t LookupSig[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x20};
constexpr uint8_t RangeSig[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57};
constexpr uint8_t RelationSig[]={0x41,0xb1,0x01,0xe9,0x28,0x43,0x00,0x00};
constexpr uint8_t LineSig[]={0x40,0x53,0x57,0x41,0x55,0x48,0x83,0xec,0x50,0x45,0x8b,0xe8};
constexpr uint8_t StatsSig[]={0x48,0x89,0x5c,0x24,0x08,0x55,0x56,0x57,0x48,0x83,0xec,0x30};
// Check the hash layout used below, and the actual attack/flags/finish call
// sites. A matching common prologue alone is insufficient for these layouts.
constexpr uint8_t HashSig[]={0x41,0x8b,0xc0,0x83,0xe0,0x7f,0x48,0x05,0xc6,0x04,0x00,0x00,0x48,0x8d,0x04,0xc2,0xc3};
constexpr uint8_t AttackCallsSig[]={0xe8,0x9c,0xb6,0x01,0x00,0x83,0x4c,0x24,0x70,0x02};
using TargetFn=uintptr_t(*)(uintptr_t,uintptr_t);
using LookupFn=uintptr_t(*)(uintptr_t,int,uint32_t);
using RangeFn=int(*)(uintptr_t,uintptr_t,int,void*);
using FlagsFn=uint16_t(*)(uintptr_t,uintptr_t,uintptr_t,int,bool,int);
TargetFn originalTarget=nullptr;LookupFn originalLookup=nullptr;RangeFn originalRange=nullptr;FlagsFn originalFlags=nullptr;
const D2RL::PluginContext* context=nullptr;const D2RL::ThreadService* threads=nullptr;
std::atomic<bool> ready{false};bool targetIn=false,lookupIn=false,rangeIn=false,flagsIn=false;
struct Server {uintptr_t game=0;uint64_t session=0;uint32_t player=0;};
std::mutex serverMutex;Server server;
struct Contact {uintptr_t game=0,attacker=0,defender=0;bool accepted=false;};
thread_local Contact contact;
std::atomic<unsigned> samples{0},contacts{0},accepted{0},rejected{0},noServer{0},intersections{0},stale{0},walls{0};
std::mutex queueMutex;Engine engine;HitSet hits;
struct Request {Sweep sweep;Sample sample;uint64_t session=0,at=0;};
std::array<Request,24> requests;size_t count=0;
Sample current;uint64_t inputSession=0;bool queued=false;
template<class T> bool Read(uintptr_t p,T& out) {
    return p>0x10000&&framevr::ReadMemory(&out,reinterpret_cast<const void*>(p),sizeof out);
}
void ObserveServer(uintptr_t game,uintptr_t unit) {
    uint32_t id=0,header[3]{};uintptr_t back=0;
    if(!game||!combat::LocalPlayerId(id)||!Read(unit,header)||header[0]!=0||header[2]!=id||!Read(unit+0xd8,back)||back!=game)return;
    const auto session=gamestate::LocalPlayer();
    std::lock_guard lock(serverMutex);server={game,session,id};
}
uintptr_t Lookup(uintptr_t game,int type,uint32_t id) {
    const auto unit=originalLookup(game,type,id);
    if(type==0&&unit)ObserveServer(game,unit);
    return unit;
}
uintptr_t Target(uintptr_t game,uintptr_t unit) {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    if(contact.game==game&&contact.attacker==unit&&
       (caller==d2rsig::Addr(0x42ffaa)||caller==d2rsig::Addr(0x43010a)))return contact.defender;
    const auto result=originalTarget(game,unit);
    ObserveServer(game,unit);if(result)ObserveServer(game,result);
    return result;
}
int Range(uintptr_t attacker,uintptr_t defender,int range,void* out) {
    // Only a geometrically confirmed VR contact replaces legacy grid reach.
    // Native line collision remains mandatory, preventing hits through walls.
    if(contact.attacker==attacker&&contact.defender==defender) {
        if(out) {const int zero=0;framevr::WriteMemory(out,&zero,sizeof zero);}
        return reinterpret_cast<int(*)(uintptr_t,uintptr_t,int)>(d2rsig::Addr(LineRva))(attacker,defender,0x804)==0;
    }
    return originalRange(attacker,defender,range,out);
}
uint16_t Flags(uintptr_t game,uintptr_t attacker,uintptr_t defender,int accuracy,bool force,int offset) {
    const auto result=originalFlags(game,attacker,defender,accuracy,force,offset);
    if(contact.game==game&&contact.attacker==attacker&&contact.defender==defender&&
       reinterpret_cast<uintptr_t>(_ReturnAddress())==d2rsig::Addr(0x430084))contact.accepted=(result&1)!=0;
    return result;
}
bool Position(uintptr_t unit,Vec& out) {
    uintptr_t path=0;uint16_t p[4]{};
    if(!Read(unit+0x38,path)||!Read(path,p))return false;
    out={2*(p[1]+p[0]/65536.f),0,2*(p[3]+p[2]/65536.f)};return true;
}
bool Allowed(const Request& r) {
    std::lock_guard lock(queueMutex);
    return current.active&&r.session==inputSession&&r.sample.epoch==current.epoch&&r.sample.equipment==current.equipment&&
        GetTickCount64()-r.at<=150&&r.session==gamestate::LocalPlayer()&&!gamestate::MenuOpen()&&
        r.sample.equipment==(gamestate::HandsKey()^(gamestate::WeaponSet()<<30));
}
// Body volumes are deliberately approximate: native ground footprint, with
// height inferred from MonStats2's legacy artwork height. Units are world
// metres; a typical 80px monster is 8 game world units tall. No copied assets.
bool BodyFor(uintptr_t unit,uint32_t classId,float upm,float floor,Body& body) {
    uint8_t bank=0,fields[16]{};Vec p{};
    if(!Position(unit,p)||!Read(unit+0x1bd,bank)||bank>3||classId>4095)return false;
    const auto row=reinterpret_cast<uintptr_t(*)(uint8_t,int)>(d2rsig::Addr(StatsRva))(bank,int(classId));
    if(!Read(row,fields)||fields[8]<1||fields[8]>8||fields[9]<1||fields[9]>8)return false;
    const float radius=std::max(.12f,float(std::max(fields[8],fields[9]))*.7f/upm);
    const float height=std::clamp((fields[13]?fields[13]:72)*.10f/upm,.4f,4.f);
    body.radius=std::min(radius,height*.45f)+.05f;
    body.bottom={p.x/upm,floor+body.radius,p.z/upm};body.top=body.bottom;
    body.top.y=floor+height-body.radius;return true;
}
void __cdecl Apply(const D2RL::PluginContext*,void*) noexcept {
    std::array<Request,24> batch;size_t n=0;
    {std::lock_guard lock(queueMutex);n=count;std::copy_n(requests.begin(),n,batch.begin());count=0;queued=false;}
    if(!n||!ready.load())return;
    Server local;{std::lock_guard lock(serverMutex);local=server;}
    uint32_t id=0;
    if(!combat::LocalPlayerId(id)||!local.game||local.session!=batch[0].session||local.player!=id) {++noServer;return;}
    const auto player=originalLookup(local.game,0,id);
    uint32_t playerMode=0;Vec playerPos{};
    if(!player||!Read(player+0xc,playerMode)||playerMode==0||playerMode==17||playerMode==5||playerMode==6||!Position(player,playerPos))return;
    // A bounded server-side hash walk, only during energetic swings. Native
    // units stay on the owning thread; no raw target pointer crosses threads.
    uintptr_t buckets[128]{};
    if(!Read(local.game+0x4c6*8,buckets))return;
    unsigned visited=0;
    for(uintptr_t unit:buckets) {
        for(unsigned chain=0;unit&&chain<256&&visited++<4096;++chain) {
            uintptr_t next=0;uint32_t header[4]{};
            if(!Read(unit+0x158,next)||!Read(unit,header))break;
            if(header[0]==1&&header[3]!=0&&header[3]!=12) {
                Vec pos{};
                if(Position(unit,pos)&&Length(Vec{pos.x-playerPos.x,0,pos.z-playerPos.z})<24) {
                    for(size_t j=0;j<n;++j) {
                        const auto& r=batch[j];Body body{};
                        if(!Allowed(r)){++stale;continue;}
                        // The client/server player positions may differ a little,
                        // but never accept a saved sweep after a teleport/warp.
                        if(Length(Vec{playerPos.x/r.sample.unitsPerMetre-r.sample.player.x,0,
                                      playerPos.z/r.sample.unitsPerMetre-r.sample.player.z})>.65f)continue;
                        if(!BodyFor(unit,header[1],r.sample.unitsPerMetre,r.sample.player.y,body)||!Touches(r.sweep.before,r.sweep.after,body))continue;
                        ++intersections;
                        if(!reinterpret_cast<int(*)(uintptr_t,uintptr_t,uintptr_t)>(d2rsig::Addr(RelationRva))(local.game,player,unit))continue;
                        if(reinterpret_cast<int(*)(uintptr_t,uintptr_t,int)>(d2rsig::Addr(LineRva))(player,unit,0x804)!=0){++walls;continue;}
                        if(!hits.Insert(r.session,r.sweep.stroke,header[2]))continue;
                        ++contacts;
                        const auto saved=contact;contact={local.game,player,unit,false};
                        // Native basic attack owns its nontrivial damage object,
                        // proc events, criticals, resistances, leech, XP and item
                        // durability. TLS redirects only THIS invocation's target.
                        reinterpret_cast<int(*)(uintptr_t,uintptr_t,int,int)>(d2rsig::Addr(AttackRva))(local.game,player,0,1);
                        if(contact.accepted){++accepted;framevr::MeleeImpact();}else ++rejected;
                        contact=saved;
                        break;
                    }
                }
            }
            unit=next;
        }
    }
}
}
void Init(const D2RL::PluginContext* ctx) {
    context=ctx;
    if(ctx->QueryService(&threads)!=D2RL::ServiceQueryResult::Success)threads=nullptr;
}
bool LocalServer(uintptr_t& game,uintptr_t& player) {
    Server local;{std::lock_guard lock(serverMutex);local=server;}
    uint32_t id=0;
    if(!ready.load()||!combat::LocalPlayerId(id)||!local.game||local.player!=id||
       local.session!=gamestate::LocalPlayer())return false;
    player=originalLookup(local.game,0,id);game=local.game;
    uint32_t header[3]{};uintptr_t back=0;
    return player&&Read(player,header)&&header[0]==0&&header[2]==id&&Read(player+0xd8,back)&&back==game;
}
uintptr_t ServerUnit(uintptr_t game,int type,uint32_t id) {
    return ready.load()&&game?originalLookup(game,type,id):0;
}
void Install() {
    if(ready.load()||!context||!threads||!threads->runOnGameThread)return;
    if(!d2rsig::Check(AttackRva,AttackSig,sizeof AttackSig)||!d2rsig::Check(RelationRva,RelationSig,sizeof RelationSig)||
       !d2rsig::Check(LineRva,LineSig,sizeof LineSig)||!d2rsig::Check(StatsRva,StatsSig,sizeof StatsSig)||
       !d2rsig::Check(0x4925fe,HashSig,sizeof HashSig)||!d2rsig::Check(0x43007f,AttackCallsSig,sizeof AttackCallsSig))return;
    if(!targetIn)targetIn=d2rsig::Hook(TargetRva,TargetSig,sizeof TargetSig,reinterpret_cast<void*>(&Target),reinterpret_cast<void**>(&originalTarget));
    if(!lookupIn)lookupIn=d2rsig::Hook(LookupRva,LookupSig,sizeof LookupSig,reinterpret_cast<void*>(&Lookup),reinterpret_cast<void**>(&originalLookup));
    if(!rangeIn)rangeIn=d2rsig::Hook(RangeRva,RangeSig,sizeof RangeSig,reinterpret_cast<void*>(&Range),reinterpret_cast<void**>(&originalRange));
    // Filled from the verified code snapshot, same signature used in tests.
    constexpr uint8_t flagSig[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x56,0x57,0x41,0x55,0x41,0x56};
    if(!flagsIn)flagsIn=d2rsig::Hook(FlagsRva,flagSig,sizeof flagSig,reinterpret_cast<void*>(&Flags),reinterpret_cast<void**>(&originalFlags));
    if(targetIn&&lookupIn&&rangeIn&&flagsIn&&!ready.exchange(true))
        framevr::logging::Write("Frame melee: free-swing contact hooks ready; swept capsules -> exact native target; no grip/button/auto-aim");
}
void Cancel() {
    std::lock_guard lock(queueMutex);current.active=false;count=0;engine.Reset();
}
void Submit(const Sample& sample) {
    bool schedule=false;
    {
        std::lock_guard lock(queueMutex);
        current=sample;
        const auto session=gamestate::LocalPlayer();
        if(session!=inputSession){inputSession=session;engine.Reset();count=0;queued=false;}
        Sweep sweep;
        if(!session||!ready.load()||!sample.active){current.active=false;engine.Reset();count=0;return;}
        if(engine.Step(sample,sweep)) {
            if(count==requests.size()){count=0;engine.Reset();return;} // never replay a backlog
            requests[count++]={sweep,sample,session,GetTickCount64()};++samples;
        }
        if(count&&!queued){queued=true;schedule=true;}
    }
    if(schedule&&threads->runOnGameThread(context,&Apply,nullptr)!=D2RL::Threads::Result::Success) {
        std::lock_guard lock(queueMutex);queued=false;count=0;
    }
}
void Report() {
    const auto s=samples.exchange(0),c=contacts.exchange(0),a=accepted.exchange(0),r=rejected.exchange(0),g=noServer.exchange(0);
    const auto i=intersections.exchange(0),x=stale.exchange(0),w=walls.exchange(0);
    Diagnostics d;{std::lock_guard lock(queueMutex);d=engine.TakeDiagnostics();}
    if(!s&&!c&&!g&&!d.discontinuities&&!d.recovering)return;
    char line[320];snprintf(line,sizeof line,"Frame melee: sweep samples=%u physical contacts=%u native accepted=%u rejected=%u missing server=%u intersections=%u stale=%u walls=%u pose resets=%u recovering=%u",s,c,a,r,g,i,x,w,d.discontinuities,d.recovering);
    framevr::logging::Write(line);
}
}
