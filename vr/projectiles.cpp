// D2R 3.3.93787: client/server missile creation, path velocity, and the HD
// release-point handoff. Kept separate from posing the held bow.
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include "projectiles.h"
#include "projectile_rules.h"
#include "sigscan.h"
#include "memory.h"
#include "logging.h"
#include "combat.h"
#include "melee.h"
#include "bow.h"
#include "crossbow.h"
#include "../steamvr/telegraph.h"
#include "../steamvr/aim_trace.h"

namespace gamestate {uint64_t LocalPlayer();}

namespace projectiles {
namespace {
constexpr uint64_t ClientRva=0x1B7760,ServerRva=0x5371A0,VelocityRva=0x342AE0;
constexpr uint64_t ReleaseRva=0x771A10,RemoveReleaseRva=0x70DC70,OwnerRva=0x09A7B0;
constexpr uint64_t ReleaseStorageRva=0x6E3CD0,RegistryRva=0x2677170;
constexpr uint64_t TransformStorageRva=0x2035b0,TransformParentRva=0xe3a94c,InstanceEntitiesRva=0x7507f1;
constexpr uint8_t TransformStorageSig[]={0x48,0x89,0x5c,0x24,0x20,0x56,0x57,0x41,0x56,0x48,0x83,0xec,0x40,0x48,0x8b,0xf9,0xe8,0x2b,0x83,0x00,0x00};
constexpr uint8_t TransformParentSig[]={0x44,0x8b,0xb7,0xc0,0x00,0x00,0x00};
constexpr uint64_t TransformStrideRva=0xe3a998;
constexpr uint8_t TransformStrideSig[]={0x4c,0x69,0xc0,0xd0,0x00,0x00,0x00};
constexpr uint8_t InstanceEntitiesSig[]={0x48,0x8b,0x44,0x24,0x68,0x0f,0x29,0xb4,0x24,0xd0,0x03,0x00,0x00,0x48,0x8b,0x48,0x10,0x48,0x8b,0x40,0x18,0x48,0x89,0x4d,0xb8};
constexpr uint8_t ClientSig[]={0x89,0x54,0x24,0x10,0x55,0x57,0x41,0x55,0x41,0x57,0x48,0x8d,0x6c,0x24,0xc1};
constexpr uint8_t ServerSig[]={0x40,0x55,0x57,0x41,0x56,0x48,0x8d,0x6c,0x24,0xc0,0x48,0x81,0xec,0x40,0x01,0x00,0x00};
constexpr uint8_t VelocitySig[]={0x48,0x85,0xc9,0x74,0x55,0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20};
constexpr uint8_t ReleaseSig[]={0x40,0x55,0x53,0x56,0x57,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0xac,0x24,0x90,0xfd,0xff,0xff};
constexpr uint8_t RemoveSig[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x20};
constexpr uint8_t OwnerSig[]={0x40,0x53,0x48,0x83,0xec,0x20,0xba,0x00,0x04,0x00,0x00,0x48,0x8b,0xd9,0xe8,0x3d,0x53,0x2b,0x00};
constexpr uint8_t StorageSig[]={0x48,0x89,0x5c,0x24,0x20,0x56,0x57,0x41,0x56,0x48,0x83,0xec,0x40,0x48,0x8b,0xf9,0xe8,0x9b,0x66,0x0b,0x00};

using ClientFn=uintptr_t(*)(uintptr_t,int);
using ServerFn=uintptr_t(*)(uintptr_t,uintptr_t);
using VelocityFn=void(*)(uintptr_t,int,uintptr_t,int);
using ReleaseFn=void(*)(uintptr_t,uintptr_t,uint32_t,uintptr_t);
ClientFn originalClient=nullptr;
ServerFn originalServer=nullptr;
VelocityFn originalVelocity=nullptr;
ReleaseFn originalRelease=nullptr;
std::atomic<bool> enabled{false},direct{true},speedHooksReady{false},allReady{false};
std::atomic<float> speed{2.f};
bool clientIn=false,serverIn=false,velocityIn=false,releaseIn=false;
std::atomic<unsigned> accelerated[2]{},released{},observed[2]{};
std::atomic<int> before[2]{},after[2]{};

template<class T> bool Read(uintptr_t p,T& value) {
    return p>0x10000&&framevr::ReadMemory(&value,reinterpret_cast<const void*>(p),sizeof value);
}
bool IsLocalOwner(uintptr_t owner) {
    uint32_t id=~0u,type=~0u,ownerId=~0u;
    return combat::LocalPlayerId(id)&&Read(owner,type)&&Read(owner+8,ownerId)&&SamePlayer(0,id,type,ownerId);
}
// Basic arrows, and a quiver arrow inside its own game-thread launch, share
// the configured arrow speed. Native skill casts elsewhere keep theirs.
uint32_t LocalArrowParams(uintptr_t params) {
    uint32_t missile=~0u;uintptr_t owner=0;
    if(!enabled.load()||!Read(params+0x20,missile)||!Read(params+8,owner)||!IsLocalOwner(owner))return ~0u;
    return missile==0||(missile&&missile==bowvr::QuiverLaunchMissile())?missile:~0u;
}

// Scope is on the game's actual creation thread, on BOTH client and server.
// Nesting an unrelated projectile temporarily clears it; unrelated path calls
// never inherit a previous arrow. No game-owned table or request is edited.
struct Creation {int side=-1;float multiplier=1;uintptr_t changedPath=0;uint32_t missile=0;};
// A ground skill's launch missile (telegraph::Flights) being created on the
// server: the velocity the game sets for it, live for this build and level.
struct FlightWatch {uint32_t missile=0;int velocity=0;uintptr_t unit=0;};
thread_local FlightWatch flightWatch;
thread_local Creation creation;
struct Scope {
    Creation saved;
    Scope(uintptr_t params,int side):saved(creation) {
        creation={};
        const uint32_t missile=speedHooksReady.load()?LocalArrowParams(params):~0u;
        if(missile!=~0u) {
            creation.side=side;creation.multiplier=speed.load();creation.missile=missile;++observed[side];
        }
    }
    ~Scope(){creation=saved;}
};
// Telemetry: "Frame missile flight:" where the local hero's missiles really
// end - each one's last position before it is gone - and how far they flew
// (Jeff, 2026-10-20: Cold Arrow looked short). Watched from their server
// creation only while FollowFlights runs (the target lock's scan, game thread:
// the Monster outline, Skills go to the aimed monster, or Ground skills on
// Aimed monster is On), and summed up in at most one line a second.
struct Flight {uint32_t unit=0,missile=0;float ox=0,oy=0,x=0,y=0;bool seen=false;ULONGLONG at=0;};
std::mutex flightMutex;
Flight flights[32];
std::atomic<ULONGLONG> followedAt{0};
void WatchFlight(uintptr_t created,uint32_t missile,int ox,int oy) {
    uint32_t id=0;
    if(GetTickCount64()-followedAt.load()>500||!created||!Read(created+8,id)||!id)return;   // nobody follows: record nothing
    std::lock_guard lock(flightMutex);
    Flight* slot=&flights[0];
    for(auto& f:flights)if(!f.unit){slot=&f;break;}else if(f.at<slot->at)slot=&f;   // a free slot, else the oldest
    *slot={id,missile,float(ox),float(oy),float(ox),float(oy),false,GetTickCount64()};
}
// Telemetry: "Frame wall trace:" what a Bone Wall (78) or Bone Prison (88)
// cast leaves near its cast point; their footprint and lifetime are unproven
// (PR #254, 2026-10-22). A cast (vrcam WallCast) asks; the game thread
// (FollowWalls) opens a 6 s window, at most 3 casts a game (aimtrace::Window).
// Every 100 ms of it, it walks the server's unit tables with no filter: the
// monsters' (game+0x4c6*8, 128 buckets chained at +0x158, as melee::Hostiles)
// and the objects' (game+0x546*8, as breakables::Apply). It logs each unit that
// appears within 10 cells of the cast point, and when it is gone; and each
// missile 207 the hero creates in the window (ReportSpell), followed by id
// (melee::ServerUnit(game,3,id), as FollowFlights). Nothing runs without a cast.
struct WallAsk {int skill=-1;float x=0,y=0;bool point=false;ULONGLONG at=0;};
std::mutex wallMutex;
WallAsk wallAsk;
std::atomic<bool> wallAsked{false};
std::atomic<ULONGLONG> wallUntil{0};   // the open window's end, for ReportSpell's cheap check
framevr::aimtrace::Window wallWindow;
framevr::aimtrace::Tracker<128> wallUnits;   // the baseline within 20 cells, then the new ones
float wallHeroX=0,wallHeroY=0;
int wallLines=0,wallCut=0;
constexpr int WallLineCap=40;   // new/gone lines a window; the rest counted
bool WallLine() {
    if(wallLines<WallLineCap){++wallLines;return true;}
    ++wallCut;return false;
}
// A missile 207 created by the hero inside the window: logged and followed.
void WallMissile(uintptr_t created,uintptr_t params) {
    const auto now=GetTickCount64();
    if(now>=wallUntil.load())return;   // no window: nothing
    uint32_t id=0;int points[4]{};
    if(!Read(created+8,id)||!id||!Read(params+0x24,points))return;
    ULONGLONG start=0;float cx=0,cy=0,hx=0,hy=0;
    {
        std::lock_guard lock(wallMutex);
        if(!wallWindow.Open(now))return;
        wallUnits.Seen(3,id,now,float(points[0]),float(points[1]));
        start=wallWindow.start;cx=wallWindow.x;cy=wallWindow.y;hx=wallHeroX;hy=wallHeroY;
    }
    // Its heading against the hero->cast line (0: along it, 90: across it).
    const float ax=float(points[2]-points[0]),ay=float(points[3]-points[1]),bx=cx-hx,by=cy-hy;
    const float la=std::hypot(ax,ay),lb=std::hypot(bx,by);
    const float angle=la>0&&lb>0?std::acos(std::clamp((ax*bx+ay*by)/(la*lb),-1.f,1.f))*57.29578f:-1.f;
    char line[240];snprintf(line,sizeof line,"Frame wall trace: +%llu ms missile 207 id %u created at %d,%d (%.1f cells from the cast) toward %d,%d, %.0f deg from the hero->cast line",
        (unsigned long long)(now-start),id,points[0],points[1],std::hypot(points[0]-cx,points[1]-cy),points[2],points[3],angle);
    framevr::logging::Write(line);
}
// The server's monster and object tables, every unit: f(header,x,y) in cells.
// Paths: a monster's (and any non-object's) place at path+0 (fraction, cell;
// melee::Position), an object's at path+0x10/+0x14 (breakables::Inspect).
template<class F> void EachServerUnit(uintptr_t game,F f) {
    constexpr uint32_t Tables[]={0x4c6,0x546};
    unsigned visited=0;
    for(const uint32_t table:Tables) {
        uintptr_t buckets[128]{};
        if(!Read(game+uintptr_t(table)*8,buckets))continue;
        for(uintptr_t unit:buckets)for(unsigned chain=0;unit&&chain<256&&visited++<8192;++chain) {
            uintptr_t next=0,path=0;uint32_t h[4]{};
            if(!Read(unit+0x158,next)||!Read(unit,h))break;
            float x=0,y=0;
            if(Read(unit+0x38,path)&&path>0x10000) {
                if(h[0]==2){int32_t s[2]{};if(Read(path+0x10,s)){x=float(s[0]);y=float(s[1]);}}
                else{uint16_t p[4]{};if(Read(path,p)){x=p[1]+p[0]/65536.f;y=p[3]+p[2]/65536.f;}}
            }
            if(x>0&&y>0)f(h,x,y);
            unit=next;
        }
    }
}
// Read-only diagnostics at actual native missile creation, separate from the
// earlier aim resolver. Do not claim a correct flight from the input ray alone.
void ReportSpell(uintptr_t params,int side,uintptr_t created) {
    uintptr_t owner=0;uint32_t missile=0,flags=0;int points[4]{};
    if(!enabled.load()||!Read(params+0x20,missile)||missile==0||
       !Read(params+8,owner)||!IsLocalOwner(owner))return;
    if(side==1&&created&&Read(params+0x24,points))WatchFlight(created,missile,points[0],points[1]);   // every one, not just the logged
    if(side==1&&created&&missile==207)WallMissile(created,params);   // every one, inside a wall window only
    static std::atomic<ULONGLONG> reported[2]{};
    const auto now=GetTickCount64();auto before=reported[side].load();
    if(now-before<1000||!reported[side].compare_exchange_strong(before,now))return;
    if(!Read(params,flags)||!Read(params+0x24,points))return;
    char line[240];snprintf(line,sizeof line,"Frame spell missile: %s id %u, flags %x, origin %d,%d target %d,%d, created %s",
        side?"server":"client",missile,flags,points[0],points[1],points[2],points[3],created?"yes":"no");
    framevr::logging::Write(line);
}
// A ground skill's missiles as the server actually created them (telegraph::
// Flights: Fire Blast's bomb so far): the launch re-times its circle from the
// moment it really leaves the hand, the landing ends it and teaches the speed.
// The circle itself is placed at the cast, like every ground skill's (vrcam).
void Telegraph(uintptr_t params,uintptr_t unit) {
    uintptr_t owner=0;uint32_t missile=0;int points[4]{};bool landing=false;
    if(!unit||!enabled.load()||!Read(params+0x20,missile))return;
    const int flight=framevr::telegraph::FlightOf(missile,landing);
    if(flight<0||!Read(params+8,owner)||!IsLocalOwner(owner)||!Read(params+0x24,points))return;
    auto& board=framevr::telegraph::Current();
    const auto now=GetTickCount64();
    const int skill=framevr::telegraph::Flights[flight].skill;
    char line[240];
    if(!landing) {
        const int velocity=flightWatch.unit==unit?flightWatch.velocity:0;
        const float live=framevr::telegraph::CellsPerSecondOf(velocity);
        const float speed=board.Launched(flight,float(points[0]),float(points[1]),float(points[2]),float(points[3]),now,live);
        snprintf(line,sizeof line,"Frame landing circle: skill %d missile from %d,%d to %d,%d (%.1f cells)%s, timed at %.1f cells/s (%s, game velocity %d)",skill,
            points[0],points[1],points[2],points[3],std::hypot(float(points[2]-points[0]),float(points[3]-points[1])),
            speed>0?"":" - no circle to time",speed>0?speed:board.Speed(flight),
            framevr::telegraph::SaneSpeed(live)?"live":"last flown",velocity);
    } else {
        const auto mark=board.Landed(flight,float(points[0]),float(points[1]),now);
        if(mark.cast)snprintf(line,sizeof line,"Frame landing circle: skill %d landed at %d,%d, %.1f cells from its circle %.0f,%.0f, %llu ms after the cast "
            "(%llu after launch); next guess %.1f cells/s",skill,points[0],points[1],std::hypot(points[0]-mark.x,points[1]-mark.y),mark.x,mark.y,
            (unsigned long long)(now-mark.cast),(unsigned long long)(mark.launched?now-mark.launched:0),board.Speed(flight));
        else snprintf(line,sizeof line,"Frame landing circle: skill %d landed at %d,%d with no circle showing",skill,points[0],points[1]);
    }
    // A Shock Web throws ~9 bombs: its lines with no circle at most once a second (#160 review).
    static ULONGLONG orphanAt=0;
    if(std::strstr(line,"no circle")){if(now-orphanAt<1000)return;orphanAt=now;}
    framevr::logging::Write(line);
}
uintptr_t Client(uintptr_t params,int flags) {
    std::array<uint8_t,0x98> adjusted{};framevr::crossbow::Launch bolt;
    const bool crossbow=enabled.load()&&crossbowvr::Prepare(params,adjusted,bolt);
    if(crossbow)params=reinterpret_cast<uintptr_t>(adjusted.data());
    crossbowvr::BeginClient(crossbow?&bolt:nullptr);
    Scope scope(params,0);const auto unit=bowvr::ImpactVisual(params,flags,originalClient);
    if(crossbow&&unit)bowvr::CrossbowClient(unit,bolt.flight);
    crossbowvr::EndClient();ReportSpell(params,0,unit);return unit;
}
uintptr_t Server(uintptr_t game,uintptr_t params) {
    std::array<uint8_t,0x98> adjusted{};
    const int spell=bowvr::SpellParams(game,params,adjusted);
    if(spell<0)return 0;
    if(spell>0)params=reinterpret_cast<uintptr_t>(adjusted.data());
    std::array<uint8_t,0x98> boltParams{};framevr::crossbow::Launch bolt;
    const bool crossbow=enabled.load()&&!bowvr::DirectCrossbow()&&crossbowvr::Prepare(params,boltParams,bolt,true);
    if(crossbow)params=reinterpret_cast<uintptr_t>(boltParams.data());
    const FlightWatch outer=flightWatch;
    {   // a ground skill's launch missile: watch the velocity the game gives it
        uint32_t missile=0;bool landing=false;
        flightWatch={enabled.load()&&Read(params+0x20,missile)&&framevr::telegraph::FlightOf(missile,landing)>=0&&!landing?missile:0u};
    }
    Scope scope(params,1);const auto unit=originalServer(game,params);
    if(crossbow)crossbowvr::ServerDone(bolt,unit&&bowvr::AdoptCrossbow(game,unit,bolt.flight));
    ReportSpell(params,1,unit);
    Telegraph(params,unit);
    flightWatch=outer;
    if(unit)bowvr::Created(game,params,unit);
    return unit;
}
void Velocity(uintptr_t path,int native,uintptr_t source,int line) {
    if(flightWatch.missile&&native>0) {   // the last positive value set during creation is its velocity
        uintptr_t unit=0,back=0;uint32_t type=~0u,missile=~0u;
        if(Read(path+0x40,unit)&&Read(unit,type)&&type==3&&Read(unit+4,missile)&&missile==flightWatch.missile&&Read(unit+0x38,back)&&back==path)
            {flightWatch.velocity=native;flightWatch.unit=unit;}
    }
    int adjusted=native;
    // Native creation first clears velocity, then sets the final value. Only
    // the latter is scaled, once per path per creation; slow-missile and later
    // corrections outside creation retain their normal behavior.
    if(creation.side>=0&&native>0&&path!=creation.changedPath) {
        uintptr_t unit=0,back=0;uint32_t type=~0u,missile=~0u;
        if(Read(path+0x40,unit)&&Read(unit,type)&&type==3&&Read(unit+4,missile)&&missile==creation.missile&&Read(unit+0x38,back)&&back==path) {
            adjusted=ScaledVelocity(native,creation.multiplier);
            creation.changedPath=path;
            if(adjusted!=native) {const int side=creation.side;++accelerated[side];before[side]=native;after[side]=adjusted;}
        }
    }
    originalVelocity(path,adjusted,source,line);
}

bool EntityAlive(uintptr_t registry,uint32_t entity) {
    uintptr_t begin=0,end=0;uint32_t full=0;const auto index=entity&0xfffff;
    return index!=0xfffff&&Read(registry+0x30,begin)&&Read(registry+0x38,end)&&end>=begin&&
        (end-begin)/4<=0x100000&&index<(end-begin)/4&&Read(begin+index*4,full)&&full==entity;
}
bool HasComponent(uintptr_t storage,uint32_t entity) {
    uintptr_t begin=0,end=0,page=0,packed=0,packedEnd=0;uint32_t index=0,full=0;
    const auto pageIndex=(entity>>8)&0xfff;
    if(!Read(storage+8,begin)||!Read(storage+16,end)||end<begin||(end-begin)/8>4096||pageIndex>=(end-begin)/8||
       !Read(begin+pageIndex*8,page)||!Read(page+(entity&255)*4,index))return false;
    index&=0xfffff;
    return index!=0xfffff&&Read(storage+0x20,packed)&&Read(storage+0x28,packedEnd)&&packedEnd>=packed&&
        (packedEnd-packed)/4<=0x100000&&index<(packedEnd-packed)/4&&Read(packed+index*4,full)&&full==entity;
}
void Release(uintptr_t assets,uintptr_t definition,uint32_t entity,uintptr_t unit) {
    uint32_t type=~0u,missile=~0u;
    if(enabled.load()&&Read(unit,type)&&type==3&&Read(unit+4,missile)&&
       (framevr::bowflight::BasicMissile(missile)||framevr::bowflight::ThrownMissile(missile)||missile==31||bowvr::OwnsVisual(unit))) {
        const auto owner=reinterpret_cast<uintptr_t(*)(uintptr_t)>(d2rsig::Addr(OwnerRva))(unit);
        const auto registry=d2rsig::Addr(RegistryRva);
        if(EntityAlive(registry,entity)) {
            const auto storage=reinterpret_cast<uintptr_t(*)(uintptr_t)>(d2rsig::Addr(ReleaseStorageRva))(registry);
            const bool hasReleasePoint=HasComponent(storage,entity);
            // Arrows bind the release mesh; spells bind the actual root.
            // Ice Bolt's child has no release component, so absence alone
            // can bind that child and subtract its authored height twice.
            bool root=false;
            if(!framevr::bowflight::BasicMissile(missile)&&!framevr::bowflight::ThrownMissile(missile)&&missile!=31) {
                const auto transforms=reinterpret_cast<uintptr_t(*)(uintptr_t)>(d2rsig::Addr(TransformStorageRva))(registry);
                root=VisualRoot(definition,transforms,entity,[](uintptr_t p,auto& value){return Read(p,value);});
            }
            const bool physical=bowvr::Visual(entity,unit,hasReleasePoint,root);
            if(missile==59) {
                static std::atomic<unsigned> samples{0};
                if(samples.fetch_add(1)<8) {
                    char line[160];std::snprintf(line,sizeof line,"Frame Ice Bolt visual: entity=%u instance_root=%d release_component=%d owned=%d",
                        entity,int(root),int(hasReleasePoint),int(physical));
                    framevr::logging::Write(line);
                }
            }
            if(hasReleasePoint&&(physical||(missile==0&&direct.load()&&IsLocalOwner(owner)))) {
                // Use the native removal operation, on the same renderer
                // thread as initialization. The base arrow transform/heading
                // then follows its path immediately instead of blending from
                // the hero's original animated release bone for 0.5 seconds.
                reinterpret_cast<void(*)(uintptr_t,uint32_t)>(d2rsig::Addr(RemoveReleaseRva))(registry,entity);
                ++released;
                return;
            }
        }
    }
    originalRelease(assets,definition,entity,unit);
}
}
bool Ready(){return allReady.load();}
uintptr_t CreateVisual(uintptr_t params){return originalClient?Client(params,0):0;}
void Configure(bool motionControls,float multiplier,bool directRelease) {
    enabled=motionControls;speed=std::isfinite(multiplier)?std::clamp(multiplier,1.f,3.f):1.f;direct=directRelease;
}
void Install() {
    if(!enabled.load())return;
    if(!velocityIn)velocityIn=d2rsig::Hook(VelocityRva,VelocitySig,sizeof VelocitySig,reinterpret_cast<void*>(&Velocity),reinterpret_cast<void**>(&originalVelocity));
    if(!clientIn)clientIn=d2rsig::Hook(ClientRva,ClientSig,sizeof ClientSig,reinterpret_cast<void*>(&Client),reinterpret_cast<void**>(&originalClient));
    if(!serverIn)serverIn=d2rsig::Hook(ServerRva,ServerSig,sizeof ServerSig,reinterpret_cast<void*>(&Server),reinterpret_cast<void**>(&originalServer));
    if(velocityIn&&clientIn&&serverIn&&!speedHooksReady.exchange(true))
        framevr::logging::Write("Frame bow flight: client/server velocity hooks ready (local basic arrows only)");
    if(!releaseIn&&d2rsig::Check(OwnerRva,OwnerSig,sizeof OwnerSig)&&d2rsig::Check(RemoveReleaseRva,RemoveSig,sizeof RemoveSig)&&
       d2rsig::Check(ReleaseStorageRva,StorageSig,sizeof StorageSig)&&
       d2rsig::Check(TransformStorageRva,TransformStorageSig,sizeof TransformStorageSig)&&
       d2rsig::Check(TransformParentRva,TransformParentSig,sizeof TransformParentSig)&&
       d2rsig::Check(TransformStrideRva,TransformStrideSig,sizeof TransformStrideSig)&&
       d2rsig::Check(InstanceEntitiesRva,InstanceEntitiesSig,sizeof InstanceEntitiesSig)) {
        releaseIn=d2rsig::Hook(ReleaseRva,ReleaseSig,sizeof ReleaseSig,reinterpret_cast<void*>(&Release),reinterpret_cast<void**>(&originalRelease));
        if(releaseIn)framevr::logging::Write("Frame bow flight: native release-point handoff hook ready");
    }
    allReady=releaseIn&&speedHooksReady.load();
}
// The game thread (targetlock's scan, every 100 ms): each watched missile's
// position while it lasts; the ones gone since the last line, summed up.
void FollowFlights() {
    const auto now=GetTickCount64();
    const bool resumed=now-followedAt.exchange(now)>500;
    static char ended[400]{};static int count=0;static ULONGLONG wrote=0;
    std::lock_guard lock(flightMutex);
    if(resumed){for(auto& f:flights)f={};count=0;ended[0]=0;}   // left over from before a pause: no bogus endings
    uintptr_t game=0,self=0;
    if(!melee::LocalServer(game,self))return;
    for(auto& f:flights) {
        if(!f.unit)continue;
        const uintptr_t unit=melee::ServerUnit(game,3,f.unit);
        uintptr_t path=0;uint32_t pos[2]{};
        if(unit&&Read(unit+0x38,path)&&path>0x10000&&Read(path,pos)&&pos[0]&&pos[1]) {
            f.x=pos[0]/65536.f;f.y=pos[1]/65536.f;f.seen=true;
            if(now-f.at<15000)continue;
        }
        if(!f.seen&&now-f.at<300)continue;   // not in the server's hash yet
        if(count++<4) {   // the first few in full, the rest counted
            const size_t used=std::strlen(ended);
            snprintf(ended+used,sizeof ended-used,"%smissile %u %.1f cells in %llu ms%s",used?"; ":"",
                f.missile,std::hypot(f.x-f.ox,f.y-f.oy),(unsigned long long)(now-f.at),f.seen?"":" (never seen in flight)");
        }
        f={};
    }
    if(count&&now-wrote>=1000) {
        char line[512];snprintf(line,sizeof line,"Frame missile flight: %d ended: %s%s",count,ended,count>4?"; ...":"");
        framevr::logging::Write(line);
        wrote=now;count=0;ended[0]=0;
    }
}
void WallCast(int skill,float x,float y,bool point) {
    if(!framevr::aimtrace::WallSkill(skill)||!std::isfinite(x)||!std::isfinite(y))return;
    if(GetTickCount64()<wallUntil.load())return;   // a window open: this cast is inside it
    {std::lock_guard lock(wallMutex);wallAsk={skill,x,y,point,GetTickCount64()};}
    wallAsked=true;
}
void FollowWalls() {
    if(!wallAsked.load()&&!wallUntil.load())return;   // no cast asked, no window: nothing runs
    uintptr_t game=0,self=0;
    if(!melee::LocalServer(game,self))return;
    using namespace framevr::aimtrace;
    const auto now=GetTickCount64();
    char line[256];
    std::lock_guard lock(wallMutex);
    wallWindow.Game(game);
    if(wallAsked.exchange(false)) {
        const WallAsk ask=wallAsk;
        const int before=wallWindow.used;
        uint16_t p[4]{};uintptr_t path=0;
        if(now-ask.at<1000&&Read(self+0x38,path)&&Read(path,p)&&wallWindow.Arm(ask.skill,now,ask.x,ask.y)) {
            wallUntil=now+WindowMs;wallUnits.Clear();wallLines=wallCut=0;
            wallHeroX=p[1]+p[0]/65536.f;wallHeroY=p[3]+p[2]/65536.f;   // melee::Position's read, in cells
            int base=0;
            EachServerUnit(game,[&](const uint32_t* h,float x,float y){
                if(Within(x,y,ask.x,ask.y,BaseCells)){wallUnits.Base(h[0],h[2]);++base;}
            });
            snprintf(line,sizeof line,"Frame wall trace: cast %d/%d, skill %d at %.1f,%.1f (%s), the hero at %.1f,%.1f (%.1f cells); %d units already within %.0f cells; watching %.0f s",
                wallWindow.used,CastsPerGame,ask.skill,ask.x,ask.y,ask.point?"the aimed point":"the hero's place: the game aimed it",
                wallHeroX,wallHeroY,std::hypot(ask.x-wallHeroX,ask.y-wallHeroY),base,BaseCells,WindowMs/1000.0);
            framevr::logging::Write(line);
        } else if(before>=CastsPerGame&&!wallWindow.Open(now)) {
            static uintptr_t toldGame=0;
            if(toldGame!=game){toldGame=game;framevr::logging::Write("Frame wall trace: 3 casts traced this game; no more until the next game");}
        }
    }
    if(wallWindow.Closed(now)) {
        int left=0;char ids[120]{};
        for(int i=0;i<wallUnits.count;++i) {
            const auto& u=wallUnits.units[i];
            if(u.base)continue;
            const size_t used=std::strlen(ids);
            if(left++<6)snprintf(ids+used,sizeof ids-used," %u/%u",u.type,u.id);
        }
        snprintf(line,sizeof line,"Frame wall trace: skill %d window over, %d new units still there (type/id:%s%s); %d lines cut",
            wallWindow.skill,left,left?ids:" none",left>6?" ...":"",wallCut);
        framevr::logging::Write(line);
        wallUntil=0;wallUnits.Clear();
        return;
    }
    if(!wallWindow.start){wallUntil=0;return;}   // none open (a new game dropped it): idle again
    if(!wallWindow.SampleDue(now))return;
    const ULONGLONG start=wallWindow.start;const float cx=wallWindow.x,cy=wallWindow.y;
    wallUnits.Begin();
    EachServerUnit(game,[&](const uint32_t* h,float x,float y){
        if(wallUnits.Find(h[0],h[2])<0&&!Within(x,y,cx,cy,NearCells))return;
        if(!wallUnits.Seen(h[0],h[2],now,x,y)||!WallLine())return;
        snprintf(line,sizeof line,"Frame wall trace: +%llu ms new type %u class %u id %u mode %u at %.1f,%.1f (%.1f cells from the cast: %+.1f,%+.1f)",
            (unsigned long long)(now-start),h[0],h[1],h[2],h[3],x,y,std::hypot(x-cx,y-cy),x-cx,y-cy);
        framevr::logging::Write(line);
    });
    for(int i=0;i<wallUnits.count;++i) {   // the missiles: by id (not in the tables walked)
        auto& u=wallUnits.units[i];
        if(u.type!=3)continue;
        const uintptr_t unit=melee::ServerUnit(game,3,u.id);
        uintptr_t path=0;uint32_t pos[2]{},h[3]{};
        if(unit&&Read(unit,h)&&h[0]==3&&h[2]==u.id) {
            u.seen=true;
            if(Read(unit+0x38,path)&&path>0x10000&&Read(path,pos)&&pos[0]&&pos[1]){u.x=pos[0]/65536.f;u.y=pos[1]/65536.f;}
        }
    }
    wallUnits.Gone([&](const Unit& u){
        if(!WallLine())return;
        snprintf(line,sizeof line,"Frame wall trace: +%llu ms gone type %u id %u, last at %.1f,%.1f, %llu ms after it appeared",
            (unsigned long long)(now-start),u.type,u.id,u.x,u.y,(unsigned long long)(now-u.at));
        framevr::logging::Write(line);
    });
}

void Report() {
    const unsigned c=accelerated[0].exchange(0),s=accelerated[1].exchange(0),r=released.exchange(0);
    const unsigned oc=observed[0].exchange(0),os=observed[1].exchange(0);
    if(!c&&!s&&!r&&!oc&&!os)return;
    char line[256];snprintf(line,sizeof line,"Frame bow flight: created client/server %u/%u, accelerated %u/%u (%d->%d / %d->%d), direct handoffs %u",
        oc,os,c,s,before[0].load(),after[0].load(),before[1].load(),after[1].load(),r);
    framevr::logging::Write(line);
}
}
