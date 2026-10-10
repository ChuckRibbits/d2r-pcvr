#pragma once
#include "math.h"
#include <array>
#include <cstdint>
#include <limits>

// Physical arrows use metres and a single release pose. This layer contains no
// native pointers: server, client and renderer exchange ids plus these values.
namespace framevr::bowflight {
using math::Vec;
inline float Length(Vec v) { return std::sqrt(math::Dot(v,v)); }
inline bool Finite(Vec v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
struct Shot {
    Vec origin{},direction{},player{};
    float unitsPerMetre=0,draw=0;
    uint64_t session=0,epoch=0,sequence=0,at=0;
    uint32_t equipment=0,stamp=0;
};
inline bool Valid(const Shot& s) {
    return s.session&&s.sequence&&Finite(s.origin)&&Finite(s.direction)&&Finite(s.player)&&
        std::isfinite(s.unitsPerMetre)&&s.unitsPerMetre>=1&&s.unitsPerMetre<=12&&
        std::isfinite(s.draw)&&s.draw>=0&&s.draw<=1&&
        std::fabs(Length(s.direction)-1)<.01f&&Length(s.origin-s.player)<3.5f;
}
struct Gate {
    bool active=false;
    uint64_t session=0,epoch=0,at=0;
    uint32_t equipment=0;
};
inline bool Allowed(const Shot& s,const Gate& g,uint64_t now) {
    return Valid(s)&&g.active&&s.session==g.session&&s.epoch==g.epoch&&s.equipment==g.equipment&&
        now>=s.at&&now-s.at<=150&&now>=g.at&&now-g.at<=150;
}
class Once {
    uint64_t session=0,sequence=0;
public:
    bool Take(const Shot& s,const Gate& g,uint64_t now) {
        if(!Allowed(s,g,now))return false;
        if(session==s.session&&s.sequence<=sequence)return false;
        session=s.session;sequence=s.sequence;return true;
    }
};

// Earliest point where the moving arrow tip enters a creature capsule. The
// complete segment is tested even when an arrow crosses a body in one tick.
inline bool SphereEntry(Vec from,Vec delta,Vec center,float radius,float& t) {
    const Vec q=from-center;
    const double a=math::Dot(delta,delta),b=math::Dot(q,delta),c=math::Dot(q,q)-radius*radius;
    if(c<=0){t=0;return true;}
    const double d=b*b-a*c;
    if(a<1e-12||d<0)return false;
    const double root=(-b-std::sqrt(d))/a;
    if(root<0||root>1)return false;
    t=float(root);return true;
}
inline bool CapsuleEntry(Vec from,Vec to,Vec bottom,Vec top,float radius,float& t) {
    if(!Finite(from)||!Finite(to)||!Finite(bottom)||!Finite(top)||!std::isfinite(radius)||radius<=0)return false;
    const Vec axis=top-bottom,delta=to-from,q=from-bottom;
    const float h2=math::Dot(axis,axis);
    if(h2<1e-8f)return SphereEntry(from,delta,bottom,radius,t);
    const float projection=std::clamp(math::Dot(q,axis)/h2,0.f,1.f);
    if(Length(q-axis*projection)<=radius){t=0;return true;}
    float best=std::numeric_limits<float>::infinity(),candidate=0;
    if(SphereEntry(from,delta,bottom,radius,candidate))best=candidate;
    if(SphereEntry(from,delta,top,radius,candidate))best=std::min(best,candidate);
    const Vec d=delta-axis*(math::Dot(delta,axis)/h2),p=q-axis*(math::Dot(q,axis)/h2);
    const double a=math::Dot(d,d),b=math::Dot(p,d),c=math::Dot(p,p)-radius*radius,disc=b*b-a*c;
    if(a>1e-12&&disc>=0) {
        const double root=(-b-std::sqrt(disc))/a;
        const double along=(math::Dot(q,axis)+root*math::Dot(delta,axis))/h2;
        if(root>=0&&root<=1&&along>=0&&along<=1)best=std::min(best,float(root));
    }
    if(best>1)return false;
    t=best;return true;
}

// Supercover grid traversal: sample both neighbours at corner crossings. A
// blocking/unloaded native cell is never skipped by a high-speed arrow.
template<class Blocked> float WallEntry(Vec from,Vec to,float unitsPerMetre,Blocked blocked) {
    const double scale=unitsPerMetre*.5,fx=from.x*scale,fz=from.z*scale;
    const double dx=(to.x-from.x)*scale,dz=(to.z-from.z)*scale;
    if(!Finite(from)||!Finite(to)||!std::isfinite(scale)||scale<=0||
       fx<1||fz<1||fx>65534||fz>65534)return 0;
    int x=int(std::floor(fx)),z=int(std::floor(fz));
    if(blocked(x,z))return 0;
    const int sx=dx>0?1:dx<0?-1:0,sz=dz>0?1:dz<0?-1:0;
    const double inf=std::numeric_limits<double>::infinity();
    double tx=sx?(x+(sx>0?1:0)-fx)/dx:inf,tz=sz?(z+(sz>0?1:0)-fz)/dz:inf;
    const double ax=sx?1/std::fabs(dx):inf,az=sz?1/std::fabs(dz):inf;
    for(int step=0;step<512;++step) {
        const double t=std::min(tx,tz);
        if(t>1)return 1;
        const bool both=std::fabs(tx-tz)<1e-9;
        if(both) {
            if(blocked(x+sx,z)||blocked(x,z+sz))return float(t);
            x+=sx;z+=sz;tx+=ax;tz+=az;
        } else if(tx<tz) {x+=sx;tx+=ax;} else {z+=sz;tz+=az;}
        if(x<1||z<1||x>65534||z>65534||blocked(x,z))return float(t);
    }
    return 0; // never approve an unbounded/unexamined segment
}

// Ordinary arrow velocity is 24 << 8, reduced to 75% by native creation.
// Map that to a 20 m/s baseline before the configured multiplier; the native
// Slow Missiles reduction therefore also slows physical flight.
inline float Speed(int nativeVelocity,float draw) {
    if(nativeVelocity<=0||!std::isfinite(draw))return 0;
    return std::clamp(float(nativeVelocity)*(20.f/4608.f)*(.7f+.3f*std::clamp(draw,0.f,1.f)),0.f,100.f);
}
inline Vec Tip(const Shot& s,float distance) {return s.origin+s.direction*(.70f+distance);}
struct Flight {
    Shot shot{};
    float distance=0,speed=0;
    uint64_t updatedAt=0;
    bool alive=false;
    std::array<uint32_t,64> hit{};
    size_t hits=0;
    bool Seen(uint32_t id) const {return std::find(hit.begin(),hit.begin()+hits,id)!=hit.begin()+hits;}
    bool Remember(uint32_t id) {if(Seen(id)||hits==hit.size())return false;hit[hits++]=id;return true;}
    Vec TailAt(uint64_t now) const {
        const float extra=alive&&now>=updatedAt?std::min(float(now-updatedAt)/1000.f,.04f)*speed:0;
        return shot.origin+shot.direction*(distance+extra);
    }
};
// The asset initializer visits both the unit root and its visible mesh. Only
// the mesh has TrackReleasePoint; binding the root would compose the mesh's
// authored height/scale on top of our world-space flight pose. Keep the full
// entity generation and never let a later parent/helper callback rebind it.
class VisualBinding {
    uint32_t entity=UINT32_MAX;
public:
    bool Bound() const {return entity!=UINT32_MAX;}
    bool Matches(uint32_t candidate) const {return Bound()&&entity==candidate;}
    bool Bind(uint32_t candidate,bool hasReleasePoint) {
        if(!hasReleasePoint||(candidate&0xfffff)==0xfffff)return false;
        if(Bound())return Matches(candidate);
        entity=candidate;return true;
    }
    void Clear(){entity=UINT32_MAX;}
};
// HD basic-arrow mesh: tail at local +Z=1, tip at -Z=-.98391676.
// Its entity's stock 45-degree yaw/release-bone offset is replaced, not added.
inline void ArrowMatrix(const Flight& f,uint64_t now,float out[16]) {
    const Vec back=f.shot.direction*(-1);
    const Vec guide=std::fabs(back.y)<.95f?Vec{0,1,0}:Vec{1,0,0};
    const Vec right=math::Normalized(math::Cross(guide,back)),up=math::Cross(back,right);
    const float scale=.70f*f.shot.unitsPerMetre/1.98391676f;
    const Vec origin=f.TailAt(now)*f.shot.unitsPerMetre-back*scale;
    out[0]=right.x*scale;out[1]=right.y*scale;out[2]=right.z*scale;out[3]=0;
    out[4]=up.x*scale;out[5]=up.y*scale;out[6]=up.z*scale;out[7]=0;
    out[8]=back.x*scale;out[9]=back.y*scale;out[10]=back.z*scale;out[11]=0;
    out[12]=origin.x;out[13]=origin.y;out[14]=origin.z;out[15]=1;
}
}
