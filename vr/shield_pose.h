#pragma once
#include "bow_pose.h"

namespace skel::shield {
// A tracked shield must not stop at the authored arm's reach. Extend the arm
// only after it runs out of length; a small IK margin keeps the wrist on the
// requested point. Recompute from this frame's unextended pose, never accumulate.
// The cap contains extreme tracking/calibration errors rather than amplifying them.
inline float ReachScale(bow::Vec shoulder,bow::Vec wrist,float upper,float lower) {
    const float distance=bow::Length(wrist-shoulder),length=upper+lower;
    if(!std::isfinite(distance)||!std::isfinite(length)||upper<=.01f||lower<=.01f)return 1;
    return std::clamp(distance/(length*.999f),1.f,2.f);
}
// An authored shield-to-wrist relationship, independent of attack/walk
// animation and the main weapon's adjustment. Keep its handedness and size.
inline bool Follow(const bow::Mat& bindWrist,const bow::Mat& bindShield,
                   const bow::Mat& trackedWrist,bow::Mat& result) {
    bow::Mat inverse;
    if(!bow::Inverse(bindWrist,inverse)||!bow::Affine(bindShield)||!bow::Affine(trackedWrist))return false;
    const auto local=bow::Multiply(bindShield,inverse);
    const float scale=bow::Length({local.m[0],local.m[1],local.m[2]});
    if(scale<.3f||scale>3.f)return false;
    result=bow::Multiply(local,trackedWrist);return bow::Affine(result);
}
// VR Settings > Shield: where the shield sits on the wrist it hangs from. The
// offsets are carried in the hand's own axes, so the shield stays rigid with the
// hand: moving it never changes how it turns with the wrist (about the wrist).
// 2026-10-20: their directions come from the bind pose, once per model - the
// attach bone's offset from the forearm's line, read each frame, flipped Out
// (and the shield) when that bone sat near the line, and the Cross(f,{0,1,0})
// fallback broke down with the forearm upright.
constexpr float kGripMaxCm=40,kSideMaxCm=25;
inline bool Finite(bow::Vec v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline float Clamp(float cm,float low,float high) {return std::isfinite(cm)?std::clamp(cm,low,high):0.f;}
// In the hand bone's axes: elbow to wrist as the bind pose holds the hand, and
// out from the forearm's line toward the shield's bone (a bone on the line takes
// the forearm axis most square to it), square to it.
struct Arm {bow::Vec along{},out{};bool ok=false;};
inline Arm LearnArm(const bow::Mat& bindForearm,const bow::Mat& bindHand,bow::Vec bindShield) {
    using namespace bow;
    Arm arm;Mat handInverse;
    if(!Affine(bindForearm)||!Inverse(bindHand,handInverse)||!Finite(bindShield))return arm;
    const Vec elbow=Position(bindForearm),f=Position(bindHand)-elbow;const float length=Length(f);
    if(!(length>1e-4f))return arm;
    const Vec along=f*(1/length);
    Vec side=bindShield-elbow;side=side-along*Dot(side,along);
    if(!(Length(side)>.05f*length)) {
        float best=2;
        for(int r=0;r<3;++r) {
            const Vec a=Normalized({bindForearm.m[r*4],bindForearm.m[r*4+1],bindForearm.m[r*4+2]});
            if(Length(a)>.5f&&std::fabs(Dot(a,along))<best){best=std::fabs(Dot(a,along));side=a-along*Dot(a,along);}
        }
    }
    arm.along=Normalized(Vector(handInverse,along));arm.out=Normalized(Vector(handInverse,Normalized(side)));
    arm.out=Normalized(arm.out-arm.along*Dot(arm.out,arm.along));
    arm.ok=Length(arm.along)>.5f&&Length(arm.out)>.5f;
    return arm;
}
// This frame's along, out and up (square to both), turned with the hand: an
// orthonormal set rigid with it, so it can neither vanish nor flip.
inline bool Axes(const bow::Mat& hand,const Arm& arm,bow::Vec& along,bow::Vec& out,bow::Vec& up) {
    using namespace bow;
    if(!arm.ok||!Affine(hand))return false;
    along=Normalized(Vector(hand,arm.along));out=Vector(hand,arm.out);out=Normalized(out-along*Dot(out,along));
    up=Cross(along,out);
    return Length(along)>.5f&&Length(out)>.5f;
}
// Grip up toward the elbow, Out and Up, cm, clamped to the settings' ranges;
// anything not finite is 0.
inline bow::Vec Offset(bow::Vec along,bow::Vec out,bow::Vec up,float gripCm,float outCm,float upCm,float unitsPerMetre) {
    if(!std::isfinite(unitsPerMetre)||unitsPerMetre<=0)return {};
    return (along*-Clamp(gripCm,0,kGripMaxCm)+out*Clamp(outCm,-kSideMaxCm,kSideMaxCm)+
            up*Clamp(upCm,-kSideMaxCm,kSideMaxCm))*(.01f*unitsPerMetre);
}
// For the shield trace: the turn between two poses' axes, degrees (scale and
// mirroring ignored), and a pose's heading, pitch and roll, degrees.
inline float TurnDeg(const bow::Mat& a,const bow::Mat& b) {
    float trace=0;
    for(int r=0;r<3;++r)trace+=bow::Dot(bow::Normalized({a.m[r*4],a.m[r*4+1],a.m[r*4+2]}),bow::Normalized({b.m[r*4],b.m[r*4+1],b.m[r*4+2]}));
    return std::acos(std::clamp((trace-1)*.5f,-1.f,1.f))*57.29578f;
}
inline bow::Vec Angles(const bow::Mat& m) {
    using namespace bow;
    const Vec x=Normalized({m.m[0],m.m[1],m.m[2]}),y=Normalized({m.m[4],m.m[5],m.m[6]}),z=Normalized({m.m[8],m.m[9],m.m[10]});
    return Vec{std::atan2(z.x,z.z),std::asin(std::clamp(-z.y,-1.f,1.f)),std::atan2(x.y,y.y)}*57.29578f;
}
}
