#pragma once
#include "math.h"

namespace framevr::body {
using math::Vec;
inline bool Finite(Vec v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
// Model coordinates: +X left, +Y up, +Z forward. Pitch is positive up;
// roll is positive right ear down, as published with the tracked hand sample.
inline Vec NeckTarget(Vec eye, Vec neckToEye, float yaw, float pitch, float roll) {
    const float s=std::sin(yaw),c=std::cos(yaw),sp=std::sin(pitch),cp=std::cos(pitch);
    const Vec forward{s*cp,sp,c*cp},right{-c,0,s},levelUp{-s*sp,cp,-c*sp};
    const Vec up=levelUp*std::cos(roll)+right*std::sin(roll);
    const Vec left=math::Cross(up,forward);
    return eye-(left*neckToEye.x+up*neckToEye.y+forward*neckToEye.z);
}
inline float UnitsPerMetre(float standingEye, float userHeight, float reach) {
    return std::clamp(standingEye/std::max(.8f,userHeight),2.f,8.f)*reach;
}
// A two-segment leg, using the animated knee direction as its pole. Preserve
// segment lengths; clamp unreachable targets rather than stretching the skin.
inline bool Knee(Vec hip, float thigh, float shin, Vec& foot, Vec pole, Vec& knee) {
    if(!Finite(hip)||!Finite(foot)||!Finite(pole)||!std::isfinite(thigh+shin)||thigh<=.01f||shin<=.01f)return false;
    Vec delta=foot-hip;float distance=std::sqrt(math::Dot(delta,delta));
    const float low=std::fabs(thigh-shin)+.0001f,high=(thigh+shin)*.9999f;
    if(high<=low)return false;
    Vec axis=distance>1e-5f?delta*(1.f/distance):Vec{0,-1,0};
    distance=std::clamp(distance,low,high);foot=hip+axis*distance;
    pole=pole-axis*math::Dot(pole,axis);
    if(math::Dot(pole,pole)<1e-6f) {
        const Vec fallback=std::fabs(axis.z)<.9f?Vec{0,0,1}:Vec{1,0,0};
        pole=fallback-axis*math::Dot(fallback,axis);
    }
    const float along=(thigh*thigh-shin*shin+distance*distance)/(2*distance);
    knee=hip+axis*along+math::Normalized(pole)*std::sqrt(std::max(0.f,thigh*thigh-along*along));
    return Finite(knee);
}
}
