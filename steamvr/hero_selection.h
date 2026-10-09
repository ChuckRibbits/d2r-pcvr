#pragma once
#include <cmath>

namespace framevr {
// The camera target identifies the local player's location. Corpse and remote
// player rigs share the same model folder; names alone cannot select the body.
inline bool LocalStandingBody(float modelX,float modelZ,float playerX,float playerZ,
                              float posedHeadHeight,float standingHeadHeight) {
    if(!std::isfinite(modelX)||!std::isfinite(modelZ)||!std::isfinite(playerX)||!std::isfinite(playerZ)||
       !std::isfinite(posedHeadHeight)||!std::isfinite(standingHeadHeight)||standingHeadHeight<=2)return false;
    return std::fabs(modelX-playerX)<3&&std::fabs(modelZ-playerZ)<3&&posedHeadHeight>standingHeadHeight*.35f;
}
}
