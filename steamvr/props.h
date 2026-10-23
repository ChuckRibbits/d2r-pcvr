#pragma once
#include "frame_math.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

// The level's own small props (cups, plates, vases...) that weapons knock
// about. The installer catalogues them from the player's game files
// (steamvr/prepare_props.py); vr/prop_scan.cpp finds them among the
// renderer's entities near the player; dropped_bottles.cpp simulates them;
// the renderer draws them where they land. Everything here is plain data and
// arithmetic, shared by both sides and the tests.
namespace framevr::props {
using math::Vec;
using math::Quat;

// ---- The catalog (d2r_vr_props/props.bin) ----
enum class Shape:uint8_t {Round,Box,Ball};
enum class Material:uint8_t {Ceramic,Glass,Metal,Wood,Paper,Bone,Wicker,Food,Cloth};
inline constexpr unsigned Shapes=3,Materials=9;
struct Kind {char name[28]{};Shape shape=Shape::Round;Material material=Material::Ceramic;};
struct Entry {uint32_t id=0;float x=0,y=0,z=0;uint16_t kind=0;};
inline constexpr uint32_t CatalogMagic=0x31505250,CatalogFormat=1; // 'PRP1'
// A preset's position round-trips exactly through the renderer's local
// matrix; this only absorbs float noise.
inline constexpr float MatchUnits=1e-3f;
class Catalog {
    std::vector<Entry> entries;
    std::vector<Kind> kinds;
public:
    // False for a malformed file; nothing is kept then.
    bool Parse(std::span<const uint8_t> data) {
        entries.clear();kinds.clear();
        auto u32=[&](size_t at){uint32_t v;std::memcpy(&v,data.data()+at,4);return v;};
        if(data.size()<16||u32(0)!=CatalogMagic||u32(4)!=CatalogFormat)return false;
        const size_t k=u32(8),n=u32(12);
        if(!k||k>255||n>(1u<<20)||data.size()!=16+k*32+n*20)return false;
        std::vector<Kind> nextKinds(k);std::vector<Entry> nextEntries(n);
        for(size_t i=0;i<k;++i) {
            const auto* at=data.data()+16+i*32;
            std::memcpy(nextKinds[i].name,at,27);
            if(at[28]>=Shapes||at[29]>=Materials)return false;
            nextKinds[i].shape=Shape(at[28]);nextKinds[i].material=Material(at[29]);
        }
        for(size_t i=0;i<n;++i) {
            const auto* at=data.data()+16+k*32+i*20;auto& e=nextEntries[i];
            std::memcpy(&e.id,at,4);std::memcpy(&e.x,at+4,4);std::memcpy(&e.y,at+8,4);std::memcpy(&e.z,at+12,4);std::memcpy(&e.kind,at+16,2);
            if(e.kind>=k||!std::isfinite(e.x)||!std::isfinite(e.y)||!std::isfinite(e.z)||(i&&e.id<nextEntries[i-1].id))return false;
        }
        kinds=std::move(nextKinds);entries=std::move(nextEntries);
        return true;
    }
    // The kind of the preset entity with this id placed at this local
    // position, or -1: an id alone could be another preset's entity.
    int Find(uint32_t id,const float local[3]) const {
        auto it=std::lower_bound(entries.begin(),entries.end(),id,[](const Entry& e,uint32_t v){return e.id<v;});
        for(;it!=entries.end()&&it->id==id;++it)
            if(std::fabs(it->x-local[0])<=MatchUnits&&std::fabs(it->y-local[1])<=MatchUnits&&std::fabs(it->z-local[2])<=MatchUnits)return it->kind;
        return -1;
    }
    const Kind& KindAt(unsigned i) const {static const Kind none{};return i<kinds.size()?kinds[i]:none;}
    size_t Size() const {return entries.size();}
    unsigned Kinds() const {return unsigned(kinds.size());}
};

// How each material falls and bounces. Densities are for the solid hull, so
// hollow vessels are given less than their clay.
struct Physical {float density,friction,bounce,rolling;};
inline constexpr std::array<Physical,Materials> Properties{{
    {600,.6f,.15f,.02f},  // ceramic
    {500,.5f,.3f,.015f},  // glass, like the dropped bottles
    {900,.4f,.25f,.02f},  // metal
    {500,.6f,.2f,.03f},   // wood
    {400,.7f,.05f,.05f},  // paper
    {600,.6f,.2f,.04f},   // bone
    {150,.7f,.1f,.04f},   // wicker
    {800,.6f,.1f,.04f},   // food
    {100,.9f,0,.1f},      // cloth
}};

// ---- The renderer's matrices ----
// A world matrix with row vectors: rows 0-2 are the model's scaled local
// axes, row 3 its origin (render units).
struct Placement {Vec p{};Quat q{0,0,0,1};Vec scale{1,1,1};};
inline bool Decompose(const float m[16],Placement& out) {
    for(int i=0;i<16;++i)if(!std::isfinite(m[i]))return false;
    if(std::fabs(m[3])>1e-4f||std::fabs(m[7])>1e-4f||std::fabs(m[11])>1e-4f||std::fabs(m[15]-1)>1e-4f)return false;
    const Vec r[3]{{m[0],m[1],m[2]},{m[4],m[5],m[6]},{m[8],m[9],m[10]}};
    Vec s{std::sqrt(math::Dot(r[0],r[0])),std::sqrt(math::Dot(r[1],r[1])),std::sqrt(math::Dot(r[2],r[2]))};
    if(!(s.x>1e-4f&&s.y>1e-4f&&s.z>1e-4f&&s.x<1e4f&&s.y<1e4f&&s.z<1e4f))return false;
    const Vec x=r[0]*(1/s.x),y=r[1]*(1/s.y),z=r[2]*(1/s.z);
    // Mirrored or sheared models cannot be a rigid body.
    if(std::fabs(math::Dot(x,y))>.02f||std::fabs(math::Dot(y,z))>.02f||std::fabs(math::Dot(x,z))>.02f||math::Dot(math::Cross(x,y),z)<.95f)return false;
    // Re-orthogonalize before taking the rotation.
    const Vec ox=math::Normalized(x),oz=math::Normalized(math::Cross(ox,y)),oy=math::Cross(oz,ox);
    Quat q=math::FromAxes(ox,oy,oz);
    const float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
    if(!(n>.5f))return false;
    out={{m[12],m[13],m[14]},{q.x/n,q.y/n,q.z/n,q.w/n},s};
    return true;
}
inline void Compose(const Placement& p,float m[16]) {
    const Vec x=math::Rotate(p.q,{1,0,0})*p.scale.x,y=math::Rotate(p.q,{0,1,0})*p.scale.y,z=math::Rotate(p.q,{0,0,1})*p.scale.z;
    const float v[16]{x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,p.p.x,p.p.y,p.p.z,1};
    std::memcpy(m,v,sizeof v);
}
struct Box {Vec lo{},hi{};};
inline bool Finite(Vec v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
// The world bounds of local bounds under a matrix, as the renderer keeps them.
inline Box WorldBounds(const Box& local,const float m[16]) {
    const Vec c=(local.lo+local.hi)*.5f,e=(local.hi-local.lo)*.5f;
    Vec wc{m[12],m[13],m[14]},we{};
    const float* cc=&c.x;const float* ee=&e.x;float* oc=&wc.x;float* oe=&we.x;
    for(int k=0;k<3;++k)for(int r=0;r<3;++r){oc[k]+=cc[r]*m[r*4+k];oe[k]+=ee[r]*std::fabs(m[r*4+k]);}
    return {wc-we,wc+we};
}
inline bool Near(const Box& a,const Box& b,float tolerance) {
    const float* x=&a.lo.x;const float* y=&b.lo.x;
    for(int i=0;i<6;++i)if(!(std::fabs(x[i]-y[i])<=tolerance))return false;
    return true;
}

// ---- What a prop rests on ----
// A horizontal surface under it: the floor, or a plane at y over a footprint
// (x0, z0)-(x1, z1). Units are whatever the boxes use.
struct Support {
    bool floor=true;
    float y=0,x0=0,z0=0,x1=0,z1=0;
    bool operator==(const Support&) const=default;
};
inline constexpr float FloorTouch=.05f,AboveTop=.03f,OnTop=.02f,SideTolerance=.01f; // metres
inline constexpr float SupportReach=1.5f; // metres either side of the prop at most
// Among the bounds of the entities around a prop (not other props), the most
// specific one under it: the smallest footprint containing its centre whose
// height range holds its bottom. On top of that entity the surface is its
// top; lower down (a shelf inside a cabinet) it is the prop's own bottom.
// Nothing found leaves a plane just past the prop's own footprint.
inline Support FindSupport(const Box& prop,std::span<const Box> around,float floor,float unitsPerMetre) {
    const float u=unitsPerMetre,bottom=prop.lo.y;
    const float cx=(prop.lo.x+prop.hi.x)*.5f,cz=(prop.lo.z+prop.hi.z)*.5f;
    if(bottom<=floor+FloorTouch*u)return {true,floor};
    const float own=(prop.hi.x-prop.lo.x)*(prop.hi.z-prop.lo.z);
    const Box* best=nullptr;float bestArea=0;
    for(const auto& b:around) {
        const float area=(b.hi.x-b.lo.x)*(b.hi.z-b.lo.z);
        if(!(area>=own*.8f)||cx<b.lo.x-SideTolerance*u||cx>b.hi.x+SideTolerance*u||cz<b.lo.z-SideTolerance*u||cz>b.hi.z+SideTolerance*u||
           bottom<b.lo.y-SideTolerance*u||bottom>b.hi.y+AboveTop*u)continue;
        if(!best||area<bestArea){best=&b;bestArea=area;}
    }
    const float reach=SupportReach*u;
    if(!best) {
        const float hx=(prop.hi.x-prop.lo.x)*.75f,hz=(prop.hi.z-prop.lo.z)*.75f;
        return {false,bottom,cx-hx,cz-hz,cx+hx,cz+hz};
    }
    const float y=bottom>=best->hi.y-OnTop*u?best->hi.y:bottom;
    return {false,std::max(y,floor),std::max(best->lo.x,cx-reach),std::max(best->lo.z,cz-reach),
            std::min(best->hi.x,cx+reach),std::min(best->hi.z,cz+reach)};
}

// ---- Waking a prop ----
// Does a capsule (segment a-b, radius r) reach a box with centre c and half
// extents h, in the frame of a body at (p, q)? Rounded corners count as the
// full inflated box: a near miss at a corner wakes a prop that then rests.
inline bool CapsuleTouchesBox(Vec a,Vec b,float r,Vec p,Quat q,Vec c,Vec h) {
    const Quat inverse=math::Conjugate(q);
    const Vec s=math::Rotate(inverse,a-p)-c,d=math::Rotate(inverse,b-p)-c-s;
    const float* so=&s.x;const float* dd=&d.x;const float* hh=&h.x;
    float t0=0,t1=1;
    for(int i=0;i<3;++i) {
        const float lo=-hh[i]-r,hi=hh[i]+r;
        if(std::fabs(dd[i])<1e-7f){if(so[i]<lo||so[i]>hi)return false;continue;}
        float ta=(lo-so[i])/dd[i],tb=(hi-so[i])/dd[i];
        if(ta>tb)std::swap(ta,tb);
        t0=std::max(t0,ta);t1=std::min(t1,tb);
        if(t0>t1)return false;
    }
    return true;
}
inline bool Overlap(const Box& a,const Box& b) {
    return a.lo.x<=b.hi.x&&b.lo.x<=a.hi.x&&a.lo.y<=b.hi.y&&b.lo.y<=a.hi.y&&a.lo.z<=b.hi.z&&b.lo.z<=a.hi.z;
}

// ---- Between the game side and the physics ----
inline constexpr unsigned MaxSeen=64;   // nearest props simulated, frozen until touched
inline constexpr unsigned MaxMoving=12; // props moving at once
inline constexpr unsigned MaxLost=16;
inline constexpr float MaxExtent=.8f,MinExtent=.01f; // metres: a prop's largest and smallest size
// A prop near the player, as the renderer has it now (render units).
struct Seen {
    uint32_t entity=0;
    uint16_t kind=0;
    Shape shape=Shape::Round;
    Material material=Material::Ceramic;
    float world[16]{};
    Box local{};       // the model's bounds in its own units
    Support support{};
};
struct Scan {
    uint64_t session=0,serial=0;
    float upm=0,floor=0;            // render units per metre; the floor under the player
    unsigned count=0;
    std::array<Seen,MaxSeen> seen{};
    unsigned lost=0;                // props whose matrix the game took back (an area reload)
    std::array<uint32_t,MaxLost> lostEntities{};
};
// New matrices for the props that moved, for the renderer (render units).
struct Pose {uint32_t entity=0;float world[16]{};};
struct Poses {
    uint64_t serial=0;
    unsigned count=0;
    std::array<Pose,MaxMoving> poses{};
};
}
