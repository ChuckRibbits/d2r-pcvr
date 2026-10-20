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
// 2026-10-20: a big shield sat a whole radius off the forearm and swung about
// wildly: hung from the hand, every wrist bend swung it round the wrist, by a
// lever as long as the shield is big (a buckler barely moved). The hand now
// gives only its turn; the shield's own centre - its rendered mesh, where the
// straps are - stays where the game hangs it on the forearm.
constexpr float kGripMaxCm=40,kSideMaxCm=25;
inline bool Finite(bow::Vec v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline bow::Vec Point(const bow::Mat& m,bow::Vec p) {return bow::Vector(m,p)+bow::Position(m);}
inline float Clamp(float cm,float low,float high) {return std::isfinite(cm)?std::clamp(cm,low,high):0.f;}
// Elbow to wrist and the shield's side of the forearm, in the forearm bone's
// own axes, from the bind pose: constant per model, so no direction is ever
// taken from a vector that shrinks to nothing as the arm moves.
struct Arm {bow::Vec along{},out{};bool ok=false;};
inline Arm LearnArm(const bow::Mat& bindForearm,const bow::Mat& bindWrist,bow::Vec bindCentre) {
    using namespace bow;
    Arm arm;Mat inverse;
    if(!Inverse(bindForearm,inverse)||!Affine(bindWrist)||!Finite(bindCentre))return arm;
    const Vec elbow=Position(bindForearm),f=Position(bindWrist)-elbow;const float length=Length(f);
    if(!(length>1e-4f))return arm;
    const Vec along=f*(1/length);
    Vec side=bindCentre-elbow;side=side-along*Dot(side,along);
    if(!(Length(side)>.05f*length)) {   // centre on the arm's line: the bone axis most square to it
        float best=2;
        for(int r=0;r<3;++r) {
            const Vec a=Normalized({bindForearm.m[r*4],bindForearm.m[r*4+1],bindForearm.m[r*4+2]});
            if(std::fabs(Dot(a,along))<best){best=std::fabs(Dot(a,along));side=a-along*Dot(a,along);}
        }
    }
    arm.along=Normalized(Vector(inverse,along));arm.out=Normalized(Vector(inverse,Normalized(side)));
    arm.ok=Length(arm.along)>.5f&&Length(arm.out)>.5f&&std::fabs(Dot(arm.along,arm.out))<.5f;
    return arm;
}
// Elbow to wrist, out to the shield's side, and square to both: orthonormal
// and continuous in the forearm's pose (the two are square in the bone's axes).
inline bool Axes(const bow::Mat& forearm,const Arm& arm,bow::Vec& along,bow::Vec& out,bow::Vec& up) {
    using namespace bow;
    if(!arm.ok||!Affine(forearm))return false;
    along=Normalized(Vector(forearm,arm.along));out=Vector(forearm,arm.out);out=Normalized(out-along*Dot(out,along));
    up=Cross(along,out);
    return Length(along)>.5f&&Length(out)>.5f;
}
// VR Settings > Shield, cm: up the forearm toward the elbow, out from it and
// across it; clamped to the settings' ranges, anything not finite is 0.
inline bow::Vec Offset(bow::Vec along,bow::Vec out,bow::Vec up,float gripCm,float outCm,float upCm,float unitsPerMetre) {
    if(!std::isfinite(unitsPerMetre)||unitsPerMetre<=0)return {};
    return (along*-Clamp(gripCm,0,kGripMaxCm)+out*Clamp(outCm,-kSideMaxCm,kSideMaxCm)+
            up*Clamp(upCm,-kSideMaxCm,kSideMaxCm))*(.01f*unitsPerMetre);
}
// The shield turned as the hand turns it, moved so that centre (in its own
// axes) lands on target. Turning about the centre, not the wrist.
inline bool Seat(const bow::Mat& followed,bow::Vec centre,bow::Vec target,bow::Mat& result) {
    if(!bow::Affine(followed)||!Finite(centre)||!Finite(target))return false;
    bow::Mat seated=followed;bow::Position(seated,bow::Position(followed)+target-Point(followed,centre));
    if(!bow::Affine(seated))return false;
    result=seated;return true;
}
// The shield mesh's centre in its attach bone's axes. A pose and the frame
// drawn from it can be a frame apart while the arm moves, so a reading counts
// once three in a row agree; a centre farther from the bone than the shield
// is wide is no shield's. Readings within the agreement leave it as it is.
struct Fit {bow::Vec centre{},last{};int agree=0;bool ok=false;};
inline bool Learn(Fit& fit,bow::Vec centre,float halfSize) {
    const float tolerance=.02f*halfSize;
    if(!Finite(centre)||!std::isfinite(halfSize)||halfSize<=1e-4f||bow::Length(centre)>2*halfSize){fit.agree=0;return false;}
    if(fit.agree>0&&bow::Length(centre-fit.last)<=tolerance)++fit.agree;else{fit.last=centre;fit.agree=1;}
    if(fit.agree<3||(fit.ok&&bow::Length(fit.last-fit.centre)<=tolerance))return false;
    fit.centre=fit.last;fit.ok=true;return true;
}
}
