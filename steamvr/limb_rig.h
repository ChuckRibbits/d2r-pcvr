#pragma once
#include "frame_math.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

// Bodies that give way ([interactions] body_physics): the monsters and
// townsfolk near the player as chains of capsules, one on each large bone,
// that weapons and hands push aside. A struck limb bends at its joint, the
// body leans, then both spring back to the game's animation. Drawn only:
// where a monster stands, its hit box and every game state stay the game's.
//
// The skeleton hook (vr/skeletons.cpp) chooses a model's limbs once from its
// meshes' boxes (Choose), hands them over as posed each frame (Capture) and
// bends the pose by the physics' answer (Apply). limb_physics.h simulates.
// Bone matrices are the model's own (row vectors, translation in the last
// row); the placement takes the model to the world (render units); world
// metres are render units / unitsPerMetre, as for weapons and props.
namespace framevr::limbs {
using math::Vec;using math::Quat;
inline constexpr int MaxLimbs=16;   // a body's largest bones
inline constexpr int MaxBodies=6;   // bodies simulated at once
inline constexpr int MaxBones=256;
// A bone whose box's longest side is under this share of the largest bone's
// is a finger, toe, jaw or strap: left out.
inline constexpr float SmallShare=.15f;
enum class Kind:uint8_t {None,Monster,Townsfolk};
// Monsters, hirelings and summons are the game's enemy models; townsfolk its npc models.
inline Kind KindOf(std::string_view model) {
    if(model.find("/character/enemy/")!=std::string_view::npos)return Kind::Monster;
    if(model.find("/character/npc/")!=std::string_view::npos)return Kind::Townsfolk;
    return Kind::None;
}
inline const char* Name(Kind k) {return k==Kind::Monster?"monster":k==Kind::Townsfolk?"townsperson":"body";}
inline bool Finite(Vec v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline float Length(Vec v) {return std::sqrt(math::Dot(v,v));}

// ---- Row-vector matrices (16 floats) ----
inline Vec Point(const float* m,Vec v) {
    return {v.x*m[0]+v.y*m[4]+v.z*m[8]+m[12],v.x*m[1]+v.y*m[5]+v.z*m[9]+m[13],v.x*m[2]+v.y*m[6]+v.z*m[10]+m[14]};
}
inline Vec Direction(const float* m,Vec v) {
    return {v.x*m[0]+v.y*m[4]+v.z*m[8],v.x*m[1]+v.y*m[5]+v.z*m[9],v.x*m[2]+v.y*m[6]+v.z*m[10]};
}
using Mat=std::array<float,16>;
inline Mat Identity() {Mat m{};m[0]=m[5]=m[10]=m[15]=1;return m;}
// a, then b.
inline Mat Mul(const float* a,const float* b) {
    Mat r{};
    for(int i=0;i<4;++i)for(int k=0;k<4;++k)for(int j=0;j<4;++j)r[i*4+j]+=a[i*4+k]*b[k*4+j];
    return r;
}
// The 3x3 part's inverse, in place of it; false when it does not invert.
inline bool Invert3(const float* m,float out[9]) {
    const float a=m[0],b=m[1],c=m[2],d=m[4],e=m[5],f=m[6],g=m[8],h=m[9],k=m[10];
    const float A=e*k-f*h,B=f*g-d*k,C=d*h-e*g,det=a*A+b*B+c*C;
    if(!std::isfinite(det)||std::fabs(det)<1e-12f)return false;
    const float s=1/det;
    out[0]=A*s;out[1]=(c*h-b*k)*s;out[2]=(b*f-c*e)*s;
    out[3]=B*s;out[4]=(a*k-c*g)*s;out[5]=(c*d-a*f)*s;
    out[6]=C*s;out[7]=(b*g-a*h)*s;out[8]=(a*e-b*d)*s;
    return true;
}
// A rotation (q turns column vectors) as the row-vector matrix that does the same.
inline void RowRotation(Quat q,float r[9]) {
    const Vec x=math::Rotate(q,{1,0,0}),y=math::Rotate(q,{0,1,0}),z=math::Rotate(q,{0,0,1});
    r[0]=x.x;r[1]=x.y;r[2]=x.z;r[3]=y.x;r[4]=y.y;r[5]=y.z;r[6]=z.x;r[7]=z.y;r[8]=z.z;
}
// A bone's turn: its x axis, and its y made square to it. Mirrored bones
// (a negative determinant) get the right-handed frame of the same x and y.
inline Quat Frame(Vec x,Vec y) {
    const Vec ax=math::Normalized(x);
    const Vec ay=math::Normalized(y-ax*math::Dot(y,ax));
    if(math::Dot(ax,ax)<.5f||math::Dot(ay,ay)<.5f)return {0,0,0,1};
    const Quat q=math::FromAxes(ax,ay,math::Cross(ax,ay));
    const float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
    return n>1e-6f&&std::isfinite(n)?Quat{q.x/n,q.y/n,q.z/n,q.w/n}:Quat{0,0,0,1};
}

// ---- Choosing a model's limbs ----
// A bone's box in its own frame: every mesh's box bound to that bone.
struct Box {
    Vec lo{},hi{};bool any=false;
    void Add(Vec l,Vec h) {
        if(!Finite(l)||!Finite(h)||l.x>h.x||l.y>h.y||l.z>h.z)return;
        if(!any){lo=l;hi=h;any=true;return;}
        lo={std::min(lo.x,l.x),std::min(lo.y,l.y),std::min(lo.z,l.z)};
        hi={std::max(hi.x,h.x),std::max(hi.y,h.y),std::max(hi.z,h.z)};
    }
    float Longest() const {return any?std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z}):0;}
    float Volume() const {return any?(hi.x-lo.x)*(hi.y-lo.y)*(hi.z-lo.z):0;}
};
// A model's limbs: the bone each sits on, its parent limb (-1 the root, which
// comes first; parents before children) and its capsule in the bone's frame.
struct Layout {
    int count=0;
    std::array<int16_t,MaxLimbs> bone{},parent{};
    std::array<Vec,MaxLimbs> a{},b{};
    std::array<float,MaxLimbs> radius{};
    int LimbOf(int bone) const {for(int i=0;i<count;++i)if(this->bone[size_t(i)]==bone)return i;return -1;}
};
// A box as a capsule along its longest side: as thick as the mean of the
// other two half sides, its ends that far in from the box's ends.
inline void Capsule(const Box& box,Vec& a,Vec& b,float& radius) {
    const Vec c=(box.lo+box.hi)*.5f,h=(box.hi-box.lo)*.5f;
    const float s[3]{h.x,h.y,h.z};
    const int k=s[0]>=s[1]&&s[0]>=s[2]?0:s[1]>=s[2]?1:2;
    radius=std::max((s[(k+1)%3]+s[(k+2)%3])*.5f,1e-4f);
    const float reach=std::max(s[k]-radius,0.f);
    const Vec axis{k==0?1.f:0.f,k==1?1.f:0.f,k==2?1.f:0.f};
    a=c-axis*reach;b=c+axis*reach;
}
// The largest bones with boxes, up to MaxLimbs. A limb's parent is its nearest
// chosen ancestor; chosen bones with none hang from the one nearest the
// skeleton's root (the largest of those), so the body is one chain.
// parents[i] < i for every bone (Granny's order); anything else is refused.
inline Layout Choose(const int16_t* parents,int bones,const Box* boxes) {
    Layout out;
    if(bones<1||bones>MaxBones)return out;
    std::array<int16_t,MaxBones> depth{};
    float longest=0;
    for(int i=0;i<bones;++i) {
        if(parents[i]>=i)return out;
        depth[size_t(i)]=parents[i]<0?0:int16_t(depth[size_t(parents[i])]+1);
        longest=std::max(longest,boxes[i].Longest());
    }
    if(!(longest>0))return out;
    std::array<int16_t,MaxBones> pick{};int picked=0;
    for(int i=0;i<bones;++i)if(boxes[i].any&&boxes[i].Longest()>=longest*SmallShare&&boxes[i].Volume()>0)pick[size_t(picked++)]=int16_t(i);
    std::sort(pick.begin(),pick.begin()+picked,[&](int16_t x,int16_t y){return boxes[x].Volume()>boxes[y].Volume();});
    picked=std::min(picked,MaxLimbs);
    std::sort(pick.begin(),pick.begin()+picked);
    if(!picked)return out;
    std::array<bool,MaxBones> chosen{};
    for(int i=0;i<picked;++i)chosen[size_t(pick[size_t(i)])]=true;
    auto ancestor=[&](int b){for(int p=parents[b];p>=0;p=parents[p])if(chosen[size_t(p)])return p;return -1;};
    int root=-1;
    for(int i=0;i<picked;++i) {
        const int b=pick[size_t(i)];
        if(ancestor(b)>=0)continue;
        if(root<0||depth[size_t(b)]<depth[size_t(root)]||(depth[size_t(b)]==depth[size_t(root)]&&boxes[b].Volume()>boxes[root].Volume()))root=b;
    }
    std::array<int16_t,MaxBones> limbOf{};limbOf.fill(-1);
    auto add=[&](int b) {
        const int i=out.count++;
        out.bone[size_t(i)]=int16_t(b);limbOf[size_t(b)]=int16_t(i);
        const int up=b==root?-1:ancestor(b);
        out.parent[size_t(i)]=int16_t(b==root?-1:up>=0?limbOf[size_t(up)]:0);
        Capsule(boxes[b],out.a[size_t(i)],out.b[size_t(i)],out.radius[size_t(i)]);
    };
    add(root);
    for(int i=0;i<picked;++i)if(pick[size_t(i)]!=root)add(pick[size_t(i)]);
    return out;
}

// ---- One frame's pose, for the physics ----
// A limb as posed (world metres): its joint (the bone's origin), the bone's
// turn and its capsule.
struct Limb {Vec joint{};Quat q{0,0,0,1};Vec a{},b{};float radius=0;};
struct Rig {
    uint32_t entity=0;
    Kind kind=Kind::None;
    uint64_t layout=0;   // which model's limbs: a body with another is rebuilt
    uint64_t at=0;       // GetTickCount64 of the pose
    int count=0;
    std::array<int16_t,MaxLimbs> parent{};
    std::array<Limb,MaxLimbs> limbs{};
};
inline bool Capture(const Layout& layout,const float* bones,int boneCount,const float* placement,float upm,Rig& out) {
    if(layout.count<1||!(upm>=1&&upm<=12))return false;
    out.count=0;
    for(int i=0;i<layout.count;++i) {
        const int bone=layout.bone[size_t(i)];
        if(bone<0||bone>=boneCount)return false;
        const Mat w=Mul(bones+size_t(bone)*16,placement);
        const Vec x{w[0],w[1],w[2]},y{w[4],w[5],w[6]},z{w[8],w[9],w[10]};
        auto& l=out.limbs[size_t(i)];
        l.joint=Vec{w[12],w[13],w[14]}*(1/upm);
        l.q=Frame(x,y);
        l.a=Point(w.data(),layout.a[size_t(i)])*(1/upm);l.b=Point(w.data(),layout.b[size_t(i)])*(1/upm);
        l.radius=layout.radius[size_t(i)]*(Length(x)+Length(y)+Length(z))/(3*upm);
        if(!Finite(l.joint)||!Finite(l.a)||!Finite(l.b)||!std::isfinite(l.radius)||!(l.radius>0))return false;
        out.parent[size_t(i)]=layout.parent[size_t(i)];
    }
    out.count=layout.count;
    return true;
}

// ---- The physics' answer, back into the pose ----
// Each limb's turn away from its animated one, in the limb's own frame, and
// how far the whole body was shoved, in the root limb's frame (metres): a
// newer pose than the physics saw carries both with it. A child that moves
// only with its parent has the parent's turn, seen from its own frame.
struct Reaction {
    uint32_t entity=0;
    uint64_t layout=0,at=0;
    int count=0;
    std::array<Quat,MaxLimbs> turn{};
    Vec shift{};
};
// A rotation about a point, in the model's frame (row vectors).
inline Mat About(Vec joint,const float r[9]) {
    Mat m=Identity();
    for(int i=0;i<3;++i)for(int j=0;j<3;++j)m[size_t(i*4+j)]=r[i*3+j];
    const Vec turned=Direction(m.data(),joint);
    m[12]=joint.x-turned.x;m[13]=joint.y-turned.y;m[14]=joint.z-turned.z;
    return m;
}
// The game's pose bent: each limb turned about its joint as it is carried by
// its parent limb, every bone below it with it, the whole model shoved. The
// world's turns become the model's through the placement (any scale cancels).
inline bool Apply(const Layout& layout,const int16_t* parents,float* bones,int boneCount,const float* placement,float upm,const Reaction& r) {
    if(r.count!=layout.count||layout.count<1||boneCount<1||boneCount>MaxBones||!(upm>=1&&upm<=12))return false;
    float inverse[9];
    if(!Invert3(placement,inverse))return false;
    const float x3[9]{placement[0],placement[1],placement[2],placement[4],placement[5],placement[6],placement[8],placement[9],placement[10]};
    auto toModel=[&](Quat q,float out[9]) {   // X . R . X^-1
        float r9[9],t[9];RowRotation(q,r9);
        for(int i=0;i<3;++i)for(int j=0;j<3;++j){float s=0;for(int k=0;k<3;++k)s+=x3[i*3+k]*r9[k*3+j];t[i*3+j]=s;}
        for(int i=0;i<3;++i)for(int j=0;j<3;++j){float s=0;for(int k=0;k<3;++k)s+=t[i*3+k]*inverse[k*3+j];out[i*3+j]=s;}
    };
    // Each limb's turn in the world now: its own-frame turn seen from where
    // this pose has the limb.
    std::array<Quat,MaxLimbs> turned{};Quat rootFrame{0,0,0,1};
    for(int i=0;i<layout.count;++i) {
        const int bone=layout.bone[size_t(i)];
        if(bone<0||bone>=boneCount)return false;
        const Mat w=Mul(bones+size_t(bone)*16,placement);
        const Quat q=Frame({w[0],w[1],w[2]},{w[4],w[5],w[6]});
        if(!i)rootFrame=q;
        turned[size_t(i)]=math::Multiply(math::Multiply(q,r.turn[size_t(i)]),math::Conjugate(q));
    }
    const Vec s=math::Rotate(rootFrame,r.shift)*upm;
    Mat shove=Identity();
    shove[12]=s.x*inverse[0]+s.y*inverse[3]+s.z*inverse[6];
    shove[13]=s.x*inverse[1]+s.y*inverse[4]+s.z*inverse[7];
    shove[14]=s.x*inverse[2]+s.y*inverse[5]+s.z*inverse[8];
    std::array<Mat,MaxLimbs> limb{};
    for(int i=0;i<layout.count;++i) {
        const int bone=layout.bone[size_t(i)],up=layout.parent[size_t(i)];
        if(bone<0||bone>=boneCount||up>=i)return false;
        const Mat& carried=up<0?shove:limb[size_t(up)];
        const Quat parentTurn=up<0?Quat{0,0,0,1}:turned[size_t(up)];
        float turn[9];toModel(math::Multiply(turned[size_t(i)],math::Conjugate(parentTurn)),turn);
        const float* m=bones+size_t(bone)*16;
        const Vec joint=Point(carried.data(),{m[12],m[13],m[14]});
        limb[size_t(i)]=Mul(carried.data(),About(joint,turn).data());
        for(float v:limb[size_t(i)])if(!std::isfinite(v))return false;
    }
    std::array<Mat,MaxBones> moved;
    std::array<int16_t,MaxBones> limbOf{};limbOf.fill(-1);
    for(int i=0;i<layout.count;++i)limbOf[size_t(layout.bone[size_t(i)])]=int16_t(i);
    for(int b=0;b<boneCount;++b) {
        const int up=parents[b];
        if(up>=b)return false;
        moved[size_t(b)]=limbOf[size_t(b)]>=0?limb[size_t(limbOf[size_t(b)])]:up>=0?moved[size_t(up)]:shove;
    }
    for(int b=0;b<boneCount;++b) {
        const Mat bent=Mul(bones+size_t(b)*16,moved[size_t(b)].data());
        std::copy(bent.begin(),bent.end(),bones+size_t(b)*16);
    }
    return true;
}
}
