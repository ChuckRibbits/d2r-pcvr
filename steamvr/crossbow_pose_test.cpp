#include <windows.h>
#include "../vr/crossbow.h"
#include "../shared/d2r_vr_state.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
using namespace framevr::crossbow;
static Pose pose;
static uint64_t session=7;
static uint32_t equipment=42,type=D2RVR_TYPE_CROSSBOW;
static bool menu=false,havePose=true;
static uint64_t renderTime=100;
namespace framevr {uint64_t BowRenderTime(){return renderTime;}}
namespace gamestate {
uint64_t LocalPlayer(){return session;}uint32_t HandsKey(){return equipment;}
uint32_t WeaponSet(){return 0;}uint32_t WeaponType(){return type;}bool MenuOpen(){return menu;}
}
namespace framevr::logging {void Write(const char*){}}
namespace combat {bool LocalPlayerId(uint32_t& id){id=0;return true;}}
namespace skel {bool CrossbowPose(framevr::crossbow::Pose& out){out=pose;return havePose;}}
template<size_t N,class T> static void Put(std::array<uint8_t,N>& b,size_t at,T value){std::memcpy(b.data()+at,&value,sizeof value);}
template<class T,size_t N> static T Get(const std::array<uint8_t,N>& b,size_t at){T v;std::memcpy(&v,b.data()+at,sizeof v);return v;}
static bool Near(Vec a,Vec b){return skel::bow::Length(a-b)<.004f;}
int main() {
    // Full item-local, weapon-adjust and world transforms, including a mirrored
    // attachment and nonzero world coordinates typical of Acts II/III.
    for(float scale:{.7f,1.f,1.4f})for(float sign:{-1.f,1.f})for(Vec heading:{Vec{0,0,1},Vec{1,.2f,1},Vec{-1,-.3f,0}}) {
        Mat local=skel::bow::Identity();Position(local,{.18f,.13f,1.f});
        Mat attach=skel::bow::Rotation({0,0,1},heading);
        for(int row=0;row<3;++row)for(int col=0;col<3;++col)attach.m[row*4+col]*=scale*(row==0?sign:1.f);
        Position(attach,{22600,8,15600});
        const Mat world=Multiply(local,attach);Line line;assert(FromBolt(world,line));
        const auto muzzle=Muzzle(line);
        assert(Near(muzzle,Position(world)+Vector(world,{0,0,BoltTip})));
        const Vec palm=Position(attach),support=Support(line,palm,.3f,4.f);
        const Vec centered=support+line.up*.12f;
        assert(skel::bow::Length(framevr::math::Cross(centered-line.center,line.direction))<.005f);
        assert(std::fabs(framevr::math::Dot(support-palm,line.direction)-1.2f)<.005f);
        Pose p{muzzle,line.direction,{22600,2,15600},line.scale,100,7,42,3,4};
        Launch launch;assert(Aim(p,launch));Mat visual;
        const Vec start{2.f*launch.x,0,2.f*launch.y};assert(Matrix(launch,start,visual));
        assert(Near(Position(visual)+Vector(visual,{0,0,BoltTip}),muzzle));
        const Vec move{3,0,-2};assert(Matrix(launch,start+move,visual));
        assert(Near(Position(visual)+Vector(visual,{0,0,BoltTip}),muzzle+move));
        assert(Fresh(p,150,7,42,3)&&!Fresh(p,201,7,42,3)&&!Fresh(p,99,7,42,3));
        assert(!Fresh(p,150,8,42,3)&&!Fresh(p,150,7,43,3)&&!Fresh(p,150,7,42,4));
    }
    Line line;Mat invalid=skel::bow::Identity();invalid.m[10]=0;assert(!FromBolt(invalid,line));
    invalid=skel::bow::Identity();invalid.m[5]=0;assert(!FromBolt(invalid,line));
    invalid=skel::bow::Identity();invalid.m[12]=std::numeric_limits<float>::quiet_NaN();assert(!FromBolt(invalid,line));
    // Exercise the actual native adapter against synthetic, writable memory.
    // A player ID of zero is valid; same-ID monsters and other players are not.
    std::array<uint8_t,0x98> original{},adjusted{};std::array<uint8_t,0x100> owner{},unit{},path{};
    Put(original,0,uint32_t(0x10020));Put(original,8,reinterpret_cast<uintptr_t>(owner.data()));
    Put(original,0x20,uint32_t(31));Put(original,0x24,100);Put(original,0x28,100);Put(original,0x2c,130);Put(original,0x30,110);
    Put(original,0x3c,0);Put(original,0x40,9);Put(original,0x70,uint64_t(123456));
    pose={{11240.7f,7,11604.2f},{0,0,1},{11240,1,11604},1,GetTickCount64(),session,equipment,3,4};
    crossbowvr::Gate(true,session,equipment,3);Launch launch;
    const auto before=original;const auto ptr=reinterpret_cast<uintptr_t>(original.data());
    assert(crossbowvr::Prepare(ptr,adjusted,launch,true));assert(original==before);
    const Pose serverPose=pose;pose.muzzle.x+=4;
    assert(crossbowvr::Prepare(ptr,adjusted,launch));assert(Near(launch.pose.muzzle,serverPose.muzzle));
    pose=serverPose;
    for(size_t i=0;i<original.size();++i)if(i!=0&&(i<0x24||i>=0x34))assert(adjusted[i]==original[i]);
    assert(Get<uint32_t>(adjusted,0)==0x10021&&Get<int>(adjusted,0x24)==launch.x&&Get<int>(adjusted,0x28)==launch.y);
    assert(Get<int>(adjusted,0x2c)==launch.tx&&Get<int>(adjusted,0x30)==launch.ty);
    assert(crossbowvr::Prepare(ptr,adjusted,launch,true));
    auto replicated=adjusted;pose.muzzle.x+=4;
    assert(crossbowvr::Prepare(reinterpret_cast<uintptr_t>(replicated.data()),adjusted,launch));
    assert(Near(launch.pose.muzzle,serverPose.muzzle));pose=serverPose;
    Put(owner,0,uint32_t(1));assert(!crossbowvr::Prepare(ptr,adjusted,launch));Put(owner,0,uint32_t(0));
    Put(owner,8,uint32_t(2));assert(!crossbowvr::Prepare(ptr,adjusted,launch));Put(owner,8,uint32_t(0));
    Put(original,0x20,uint32_t(0));assert(!crossbowvr::Prepare(ptr,adjusted,launch));Put(original,0x20,uint32_t(31));
    Put(original,0x3c,7);assert(!crossbowvr::Prepare(ptr,adjusted,launch));Put(original,0x3c,0);
    menu=true;assert(!crossbowvr::Prepare(ptr,adjusted,launch));menu=false;
    havePose=false;assert(!crossbowvr::Prepare(ptr,adjusted,launch));havePose=true;
    type=D2RVR_TYPE_BOW;assert(!crossbowvr::Prepare(ptr,adjusted,launch));type=D2RVR_TYPE_CROSSBOW;
    crossbowvr::Gate(false,session,equipment,3);assert(!crossbowvr::Prepare(ptr,adjusted,launch));
    crossbowvr::Gate(true,session,equipment,4);assert(!crossbowvr::Prepare(ptr,adjusted,launch));
    crossbowvr::Gate(true,session,equipment,3);assert(crossbowvr::Prepare(ptr,adjusted,launch));
    Put(unit,0,uint32_t(3));Put(unit,4,uint32_t(31));Put(unit,8,uint32_t(55));Put(unit,0x38,reinterpret_cast<uintptr_t>(path.data()));
    Put(path,0,uint32_t(launch.x*65536u));Put(path,4,uint32_t(launch.y*65536u));
    const auto u=reinterpret_cast<uintptr_t>(unit.data());
    crossbowvr::BeginClient(&launch);
    assert(!crossbowvr::Visual(12,u,false));
    assert(crossbowvr::Visual(12,u,true)); // render init before Client returns
    crossbowvr::EndClient();crossbowvr::Created(u,launch); // no duplicate record
    assert(!crossbowvr::Visual(13,u,true)); // bind only the release mesh
    Mat out=skel::bow::Identity();std::array<uint8_t,0xd0> component{};
    crossbowvr::Transform(reinterpret_cast<uintptr_t>(&out),12,reinterpret_cast<uintptr_t>(component.data()));
    assert(Near(Position(out)+Vector(out,{0,0,BoltTip}),launch.pose.muzzle));
    assert(Near(Position(Get<Mat>(component,0x80)),Position(out))); // first frame has no false motion
    const Mat first=out;pose.muzzle.x+=20; // moving the weapon cannot move an already-fired bolt
    crossbowvr::Transform(reinterpret_cast<uintptr_t>(&out),12,reinterpret_cast<uintptr_t>(component.data()));
    assert(std::memcmp(&out,&first,sizeof out)==0);
    Put(path,0,uint32_t((launch.x+2)*65536u));
    ++renderTime;
    crossbowvr::Transform(reinterpret_cast<uintptr_t>(&out),12,reinterpret_cast<uintptr_t>(component.data()));
    assert(Near(Position(out),Position(first)+Vec{4,0,0}));
    assert(Near(Position(Get<Mat>(component,0x80)),Position(first)));
    const Mat moved=out;Put(path,0,uint32_t((launch.x+5)*65536u));
    crossbowvr::Transform(reinterpret_cast<uintptr_t>(&out),12,reinterpret_cast<uintptr_t>(component.data()));
    assert(std::memcmp(&out,&moved,sizeof out)==0); // both eyes use the same motion history
    assert(Near(Position(Get<Mat>(component,0x80)),Position(first)));
    crossbowvr::Destroyed(u);out=skel::bow::Identity();
    crossbowvr::Transform(reinterpret_cast<uintptr_t>(&out),12,reinterpret_cast<uintptr_t>(component.data()));assert(Near(Position(out),{}));
    // Nested unrelated creation never inherits the outer bolt's placement.
    crossbowvr::BeginClient(&launch);crossbowvr::BeginClient(nullptr);
    assert(!crossbowvr::Visual(14,u,true));crossbowvr::EndClient();assert(crossbowvr::Visual(14,u,true));crossbowvr::EndClient();
    Put(unit,8,uint32_t(56));crossbowvr::Created(u,launch); // native pointer reused without a kill callback
    assert(crossbowvr::Visual(15,u,true));out=skel::bow::Identity();
    crossbowvr::Transform(reinterpret_cast<uintptr_t>(&out),14,reinterpret_cast<uintptr_t>(component.data()));assert(Near(Position(out),{}));
    crossbowvr::Gate(true,++session,equipment,3);out=skel::bow::Identity();
    crossbowvr::Transform(reinterpret_cast<uintptr_t>(&out),15,reinterpret_cast<uintptr_t>(component.data()));assert(Near(Position(out),{}));
    // Room cleanup can bypass individual kills. Reclaim dead records when full;
    // never let stale render bindings consume the pool for the rest of a game.
    launch.pose.session=session;
    std::array<std::array<uint8_t,0x100>,128> oldUnits{};
    for(size_t i=0;i<oldUnits.size();++i) {
        auto& old=oldUnits[i];Put(old,0,uint32_t(3));Put(old,4,uint32_t(31));Put(old,8,uint32_t(i+100));
        const auto ptr=reinterpret_cast<uintptr_t>(old.data());crossbowvr::Created(ptr,launch);
        assert(crossbowvr::Visual(uint32_t(i+100),ptr,true));
    }
    for(auto& old:oldUnits)Put(old,0,uint32_t(0));
    crossbowvr::Created(u,launch);assert(crossbowvr::Visual(999,u,true));
    out=skel::bow::Identity();crossbowvr::Transform(reinterpret_cast<uintptr_t>(&out),100,reinterpret_cast<uintptr_t>(component.data()));
    assert(Near(Position(out),{}));
    puts("Crossbow geometry, native request preservation, ownership, lifetime and muzzle placement passed");
}
