#include "dropped_bottles.h"
#include <box3d/box3d.h>
#include <algorithm>

namespace framevr::bottles {
namespace {
b3Vec3 B(Vec v) {return {v.x,v.y,v.z};}
b3Quat B(Quat q) {return {{q.x,q.y,q.z},q.w};}
Vec F(b3Vec3 v) {return {v.x,v.y,v.z};}
Quat F(b3Quat q) {return {q.v.x,q.v.y,q.v.z,q.s};}
Quat Blend(Quat a,Quat b,float t) {
    if(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0)b={-b.x,-b.y,-b.z,-b.w};
    const Quat q{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,a.w+(b.w-a.w)*t};
    const float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
    return n>1e-6f?Quat{q.x/n,q.y/n,q.z/n,q.w/n}:a;
}
// Glass on stone: a short bounce, then a roll that rolling resistance ends.
constexpr float GlassFriction=.5f,GlassBounce=.3f,GlassRolling=.015f,Density=400; // ~0.3 kg
constexpr float FloorFriction=.7f,FloorBounce=.2f,WallFriction=.4f,WallBounce=.25f;
constexpr float FloorHalf=40; // the ground box around the physics origin, metres
constexpr int Sides=24,MaxSteps=8;
}
struct World::Impl {
    b3WorldId world=b3_nullWorldId;
    b3BodyId ground=b3_nullBodyId,walls=b3_nullBodyId;
    struct Bottle {b3BodyId body=b3_nullBodyId;double born=0;uint32_t code=0;Vec p0{},p1{};Quat q0{0,0,0,1},q1{0,0,0,1};};
    std::array<Bottle,MaxBottles> bottles{};
    unsigned count=0;
    // Box3D works near its origin: physics position = world - origin.
    uint64_t session=0;
    float floor=0,upm=0;
    Vec origin{};
    double last=0,accumulator=0;
    bool timed=false;
    Patch patch{};
    bool havePatch=false;
    std::array<Rect,MaxRects> rects{};
    unsigned rectCount=0;
    Impl() {
        auto def=b3DefaultWorldDef();
        def.gravity={0,-9.81f,0};
        def.workerCount=1; // serial: Box3D starts no threads in the game process
        world=b3CreateWorld(&def);
    }
    ~Impl() {if(b3World_IsValid(world))b3DestroyWorld(world);}
    static void Destroy(b3BodyId& id) {if(!B3_IS_NULL(id)&&b3Body_IsValid(id))b3DestroyBody(id);id=b3_nullBodyId;}
    void Remove(unsigned i) {
        Destroy(bottles[i].body);
        for(unsigned k=i+1;k<count;++k)bottles[k-1]=bottles[k];
        bottles[--count]={};
    }
    void Clear() {
        while(count)Remove(count-1);
        Destroy(walls);Destroy(ground);rectCount=0;timed=false;accumulator=0;
    }
    void Restart(const Release& r) {
        Clear();
        session=r.session;floor=r.floor;upm=r.unitsPerMetre;
        origin={std::round(r.p.x),r.floor,std::round(r.p.z)};
        auto def=b3DefaultBodyDef();def.position={0,-.5f,0};
        ground=b3CreateBody(world,&def);
        auto box=b3MakeBoxHull(FloorHalf,.5f,FloorHalf);
        auto shape=b3DefaultShapeDef();shape.baseMaterial.friction=FloorFriction;shape.baseMaterial.restitution=FloorBounce;
        b3CreateHullShape(ground,&shape,&box.base);
        BuildWalls();
    }
    void BuildWalls() {
        if(!havePatch||patch.session!=session||B3_IS_NULL(ground))return;
        std::array<Rect,MaxRects> next{};
        const unsigned n=MergeWalls(patch,next);
        if(!B3_IS_NULL(walls)&&n==rectCount&&std::equal(next.begin(),next.begin()+n,rects.begin()))return;
        Destroy(walls);rects=next;rectCount=n;
        if(!n)return;
        auto def=b3DefaultBodyDef();
        walls=b3CreateBody(world,&def);
        auto shape=b3DefaultShapeDef();shape.baseMaterial.friction=WallFriction;shape.baseMaterial.restitution=WallBounce;
        const float hy=(WallHeight+.5f)*.5f; // from half a metre below the floor
        for(unsigned i=0;i<n;++i) {
            const auto& r=rects[i];
            const auto centre=CellCorner(r.x+r.w*.5f,r.z+r.d*.5f,floor,upm)-origin;
            auto box=b3MakeOffsetBoxHull(r.w/upm,hy,r.d/upm,{centre.x,hy-.5f,centre.z});
            b3CreateHullShape(walls,&shape,&box.base);
        }
    }
    Vec Position(const Bottle& b,float alpha) const {return origin+b.p0+(b.p1-b.p0)*alpha;}
};
World::World():impl(std::make_unique<Impl>()) {}
World::~World()=default;
bool World::Ready() const {return b3World_IsValid(impl->world);}
bool World::Drop(const Release& release) {
    auto& m=*impl;
    if(!Ready()||!Valid(release))return false;
    const Vec offset=release.p-m.origin;
    if(!m.count||release.session!=m.session||std::fabs(release.floor-m.floor)>.05f||
       std::fabs(release.unitsPerMetre-m.upm)>.01f*m.upm||std::hypot(offset.x,offset.z)>FloorHalf*.5f)m.Restart(release);
    if(B3_IS_NULL(m.ground))return false;
    if(m.count==MaxBottles)m.Remove(0);
    const float radius=Radius(release.radius);
    Vec p=release.p-m.origin;
    p.y=std::max(p.y,std::max(radius,Height*.5f)+.005f); // never start inside the floor
    auto def=b3DefaultBodyDef();
    def.type=b3_dynamicBody;def.position=B(p);def.rotation=B(release.q);
    def.linearVelocity=B(release.v);def.angularVelocity=B(release.w);
    def.linearDamping=.05f;def.angularDamping=.05f;
    const auto body=b3CreateBody(m.world,&def);
    if(B3_IS_NULL(body))return false;
    auto* hull=b3CreateCylinder(Height,radius,-Height*.5f,Sides);
    if(!hull){b3DestroyBody(body);return false;}
    auto shape=b3DefaultShapeDef();shape.density=Density;
    shape.baseMaterial.friction=GlassFriction;shape.baseMaterial.restitution=GlassBounce;shape.baseMaterial.rollingResistance=GlassRolling;
    const auto made=b3CreateHullShape(body,&shape,hull); // the world keeps its own copy
    b3DestroyHull(hull);
    if(B3_IS_NULL(made)){b3DestroyBody(body);return false;}
    m.bottles[m.count++]={body,release.time,release.code,p,p,release.q,release.q};
    return true;
}
void World::Walls(const Patch& patch) {
    auto& m=*impl;
    if(!patch.session||(m.havePatch&&patch.serial==m.patch.serial&&patch.session==m.patch.session))return;
    m.patch=patch;m.havePatch=true;
    if(m.count&&patch.session==m.session)m.BuildWalls();
}
void World::Advance(double time,Vec player) {
    auto& m=*impl;
    if(!Ready()||!m.count||!std::isfinite(time)){m.timed=false;return;}
    if(!m.timed){m.last=time;m.timed=true;m.accumulator=0;}
    double dt=time-m.last;m.last=time;
    if(!(dt>=0)||dt>.25)dt=Step; // a pause or clock jump resumes in place
    m.accumulator+=dt;
    int steps=0;
    while(m.accumulator>=Step&&steps<MaxSteps) {
        for(unsigned i=0;i<m.count;++i){m.bottles[i].p0=m.bottles[i].p1;m.bottles[i].q0=m.bottles[i].q1;}
        b3World_Step(m.world,float(Step),4);
        for(unsigned i=0;i<m.count;++i) {
            const auto t=b3Body_GetTransform(m.bottles[i].body);
            m.bottles[i].p1=F(t.p);m.bottles[i].q1=F(t.q);
        }
        m.accumulator-=Step;++steps;
    }
    if(steps==MaxSteps)m.accumulator=0;
    for(unsigned i=m.count;i-->0;) {
        const auto& b=m.bottles[i];
        const Vec p=m.origin+b.p1;
        const bool lost=!Finite(p)||b.p1.y<-1||time-b.born>Lifetime||
            !Finite(player)||std::hypot(p.x-player.x,p.z-player.z)>Range;
        if(lost)m.Remove(i);
    }
}
unsigned World::Show(std::array<Shown,MaxBottles>& out) const {
    const auto& m=*impl;
    const float alpha=float(std::clamp(m.accumulator/Step,0.,1.));
    for(unsigned i=0;i<m.count;++i)out[i]={m.Position(m.bottles[i],alpha),Blend(m.bottles[i].q0,m.bottles[i].q1,alpha),m.bottles[i].code};
    return m.count;
}
bool World::WallsWanted(WallRequest& out) const {
    const auto& m=*impl;
    if(!m.count)return false;
    // The newest bottle still moving; once all rest, the newest.
    unsigned pick=m.count-1;
    for(unsigned i=m.count;i-->0;)if(b3Body_IsAwake(m.bottles[i].body)){pick=i;break;}
    out={m.session,m.origin+m.bottles[pick].p1,m.floor,m.upm};
    return true;
}
unsigned World::Count() const {return impl->count;}
unsigned World::Shapes() const {return impl->rectCount;}
void World::Reset() {auto& m=*impl;m.Clear();m.session=0;m.havePatch=false;m.patch={};}
}
