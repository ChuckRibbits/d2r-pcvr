#pragma once
#include "bow_pose.h"

namespace skel::shield {
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
}
