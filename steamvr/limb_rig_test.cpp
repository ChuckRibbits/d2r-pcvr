#include "limb_rig.h"
#include <cassert>
#include <cstdio>
#include <vector>

using namespace framevr;
using namespace framevr::limbs;
using math::Vec;using math::Quat;
namespace {
constexpr float Pi=3.14159265f;
bool Near(float a,float b,float e=1e-3f) {return std::fabs(a-b)<=e;}
bool Near(Vec a,Vec b,float e=1e-3f) {return Near(a.x,b.x,e)&&Near(a.y,b.y,e)&&Near(a.z,b.z,e);}
bool SameTurn(Quat a,Quat b,float e=1e-3f) {return std::fabs(std::fabs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)-1)<=e;}
Quat About(Vec axis,float angle) {const Vec n=math::Normalized(axis);const float s=std::sin(angle*.5f);return {n.x*s,n.y*s,n.z*s,std::cos(angle*.5f)};}
// A bone at p, turned by q (row vectors: its axes are the rows).
Mat Bone(Vec p,Quat q={0,0,0,1}) {
    Mat m=Identity();
    const Vec x=math::Rotate(q,{1,0,0}),y=math::Rotate(q,{0,1,0}),z=math::Rotate(q,{0,0,1});
    m[0]=x.x;m[1]=x.y;m[2]=x.z;m[4]=y.x;m[5]=y.y;m[6]=y.z;m[8]=z.x;m[9]=z.y;m[10]=z.z;m[12]=p.x;m[13]=p.y;m[14]=p.z;
    return m;
}
// The test model: root, pelvis, spine, head, arm, hand, finger (too small),
// and a thigh hung from the root beside the pelvis.
enum {Root,Pelvis,Spine,Head,Arm,Hand,Finger,Thigh,Bones};
const int16_t Parents[Bones]{-1,Root,Pelvis,Spine,Spine,Arm,Hand,Root};
std::array<Box,Bones> Boxes() {
    std::array<Box,Bones> b{};
    b[Pelvis].Add({-1.5f,-1,-1},{1.5f,1,1});
    b[Spine].Add({-1,0,-1},{1,4,1});
    b[Head].Add({-.8f,0,-.8f},{.8f,1.6f,.8f});
    b[Arm].Add({0,-.4f,-.4f},{3,.4f,.4f});
    b[Hand].Add({0,-.3f,-.3f},{1,.3f,.3f});
    b[Finger].Add({0,-.05f,-.05f},{.4f,.05f,.05f});
    b[Thigh].Add({-.5f,-4,-.5f},{.5f,0,.5f});
    return b;
}
std::vector<float> Pose() {
    const Vec at[Bones]{{0,0,0},{0,5,0},{0,6,0},{0,10,0},{1,9,0},{4,9,0},{5,9,0},{0,5,0}};
    std::vector<float> m;
    for(int i=0;i<Bones;++i){const Mat b=Bone(at[i]);m.insert(m.end(),b.begin(),b.end());}
    return m;
}
Vec At(const std::vector<float>& pose,int bone) {return {pose[size_t(bone)*16+12],pose[size_t(bone)*16+13],pose[size_t(bone)*16+14]};}
Vec Axis(const std::vector<float>& pose,int bone,int row) {return {pose[size_t(bone)*16+row*4],pose[size_t(bone)*16+row*4+1],pose[size_t(bone)*16+row*4+2]};}
constexpr float Upm=4;
const Quat Facing=math::Yaw(Pi/2);
// The model drawn four units a metre, turned a quarter, at (100, 0, 200).
Mat Placement() {Mat p=Bone({100,0,200},Facing);for(int i=0;i<12;++i)if(i%4!=3)p[size_t(i)]*=Upm;return p;}
Reaction Still(const Layout& l) {Reaction r;r.count=l.count;for(int i=0;i<l.count;++i)r.turn[size_t(i)]={0,0,0,1};return r;}
}
int main() {
    assert(KindOf("data/hd/character/enemy/zombie1/zombie1.model")==Kind::Monster);
    assert(KindOf("data/hd/character/npc/akara/akara.model")==Kind::Townsfolk);
    assert(KindOf("data/hd/character/player/amazon/amazon.model")==Kind::None);
    assert(KindOf("data/hd/items/weapon/sword/sword.model")==Kind::None);
    const auto boxes=Boxes();
    const Layout layout=Choose(Parents,Bones,boxes.data());
    // The largest bones, the finger left out; the pelvis is the root, the
    // thigh beside it hangs from it; parents come before children.
    {
        assert(layout.count==6);
        const int16_t bones[]{Pelvis,Spine,Head,Arm,Hand,Thigh},parents[]{-1,0,1,1,3,0};
        for(int i=0;i<6;++i)assert(layout.bone[size_t(i)]==bones[i]&&layout.parent[size_t(i)]==parents[i]);
        assert(layout.LimbOf(Finger)<0&&layout.LimbOf(Hand)==4);
        // The arm's box as a capsule along its length: 0.4 thick, its ends 0.4 in.
        assert(Near(layout.radius[3],.4f)&&Near(layout.a[3],{.4f,0,0})&&Near(layout.b[3],{2.6f,0,0}));
        // A ball where the box is a cube.
        Box cube;cube.Add({-1,-1,-1},{1,1,1});Vec a,b;float r;Capsule(cube,a,b,r);
        assert(Near(r,1)&&Near(a,b));
    }
    // A skeleton without meshes: boxes from each bone's segments to its
    // children; the root, the end points and named helpers get none.
    {
        assert(NotLimb("Bip01 L Finger0")&&NotLimb("Bip01 HeadNub")&&NotLimb("weapon_attach_R")&&!NotLimb("Bip01 L Forearm")&&!NotLimb("L_Elbow"));
        enum {R,Pel,Sp,Hd,Nub,Th,Ca,Ft,Fi,N};
        const int16_t parents[N]{-1,R,Pel,Sp,Hd,Pel,Th,Ca,Ft};
        const Vec at[N]{{0,0,0},{0,5,0},{0,6,0},{0,10,0},{0,11.5f,0},{1,5,0},{1,1,0},{1,0,0},{1,0,1}};
        std::vector<float> m;
        for(int i=0;i<N;++i){const Mat b=Bone(at[i]);m.insert(m.end(),b.begin(),b.end());}
        bool skip[N]{};skip[Nub]=skip[Fi]=true;
        std::array<Box,N> boxes{};
        SegmentBoxes(parents,N,m.data(),skip,boxes.data());
        assert(!boxes[R].any&&!boxes[Nub].any&&!boxes[Fi].any);
        assert(boxes[Pel].any&&boxes[Sp].any&&boxes[Hd].any&&boxes[Th].any&&boxes[Ca].any&&boxes[Ft].any);
        // The thigh's box runs down to the calf, a quarter of its length thick (within the limits).
        const float r=std::clamp(4*SegmentThickness,11.5f*MinThickness,11.5f*MaxThickness);
        assert(Near(boxes[Th].lo,{-r,-4-r,-r})&&Near(boxes[Th].hi,{r,r,r}));
        // The head's box reaches its end point though the end point has none.
        assert(Near(boxes[Hd].hi.y,1.5f+std::clamp(1.5f*SegmentThickness,11.5f*MinThickness,11.5f*MaxThickness)));
        const Layout l=Choose(parents,N,boxes.data());
        assert(l.count==6&&l.bone[0]==Pel&&l.LimbOf(Th)>0&&l.LimbOf(Ca)>0);
    }
    // A bone listed before its parent is refused; no boxes, no limbs.
    {
        const int16_t bad[]{-1,2,0};std::array<Box,3> b{};b[1].Add({0,0,0},{1,1,1});b[2].Add({0,0,0},{1,1,1});
        assert(Choose(bad,3,b.data()).count==0);
        std::array<Box,Bones> none{};
        assert(Choose(Parents,Bones,none.data()).count==0);
    }
    const auto pose=Pose();const Mat place=Placement();
    // Captured in world metres, turned as the model is placed.
    {
        Rig rig;
        assert(Capture(layout,pose.data(),Bones,place.data(),Upm,rig)&&rig.count==6);
        const Vec arm=(math::Rotate(Facing,Vec{1,9,0})*Upm+Vec{100,0,200})*(1/Upm);
        assert(Near(rig.limbs[3].joint,arm));
        assert(SameTurn(rig.limbs[3].q,Facing));
        assert(Near(rig.limbs[3].radius,.4f));   // the model's units are the world's metres here: drawn at four a metre
        assert(Near(rig.limbs[3].b,(math::Rotate(Facing,Vec{3.6f,9,0})*Upm+Vec{100,0,200})*(1/Upm)));
        assert(rig.parent[4]==3);
        assert(!Capture(layout,pose.data(),Bones,place.data(),0,rig));   // no scale, no pose
    }
    // No turn and no shove: the pose as the game made it.
    {
        auto bent=pose;
        assert(Apply(layout,Parents,bent.data(),Bones,place.data(),Upm,Still(layout)));
        for(size_t i=0;i<bent.size();++i)assert(Near(bent[i],pose[i],1e-4f));
    }
    // A shove in the root's frame moves every bone by it, in the model's frame.
    {
        auto bent=pose;Reaction r=Still(layout);r.shift={0,0,.1f};
        assert(Apply(layout,Parents,bent.data(),Bones,place.data(),Upm,r));
        for(int b=0;b<Bones;++b)assert(Near(At(bent,b),At(pose,b)+Vec{0,0,.1f}));
    }
    // The arm bent a quarter about its own z, the hand with it: the hand and
    // finger swing round the arm's joint; the spine and head stay.
    {
        auto bent=pose;Reaction r=Still(layout);
        r.turn[3]=r.turn[4]=About({0,0,1},Pi/2);
        assert(Apply(layout,Parents,bent.data(),Bones,place.data(),Upm,r));
        assert(Near(At(bent,Arm),{1,9,0})&&Near(At(bent,Hand),{1,12,0})&&Near(At(bent,Finger),{1,13,0}));
        assert(Near(Axis(bent,Arm,0),{0,1,0})&&Near(Axis(bent,Hand,0),{0,1,0})&&Near(Axis(bent,Finger,0),{0,1,0}));
        assert(Near(At(bent,Spine),{0,6,0})&&Near(Axis(bent,Head,0),{1,0,0})&&Near(At(bent,Thigh),{0,5,0}));
        // The hand not turned with it: carried round, but facing as before.
        r.turn[4]={0,0,0,1};bent=pose;
        assert(Apply(layout,Parents,bent.data(),Bones,place.data(),Upm,r));
        assert(Near(At(bent,Hand),{1,12,0})&&Near(Axis(bent,Hand,0),{1,0,0})&&Near(At(bent,Finger),{2,12,0}));
    }
    // The pelvis leaning carries the thigh hung from the root beside it.
    {
        auto bent=pose;Reaction r=Still(layout);
        for(int i=0;i<layout.count;++i)r.turn[size_t(i)]=About({0,0,1},.3f);
        assert(Apply(layout,Parents,bent.data(),Bones,place.data(),Upm,r));
        assert(Near(Axis(bent,Thigh,1),{-std::sin(.3f),std::cos(.3f),0})&&Near(At(bent,Root),{0,0,0}));
    }
    // An answer for another layout is not applied.
    {
        auto bent=pose;Reaction r=Still(layout);r.count=3;
        assert(!Apply(layout,Parents,bent.data(),Bones,place.data(),Upm,r));
    }
    std::puts("limb rig: all checks passed");
    return 0;
}
