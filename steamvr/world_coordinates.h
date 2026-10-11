#pragma once
#include "frame_math.h"

namespace framevr::coordinates {
using math::Vec;
// D2R 3.3.93787's renderer shears elevated ground along both horizontal
// axes: cot(30 degrees) / sqrt(2) = sqrt(3/2). Dynamic-path coordinates
// (already doubled into world units) lie on the unsheared grid plane.
// Verified against paired native/render positions in the Windows cave.
// Both conversions also work in metres when every argument uses metres.
inline constexpr float GroundShear=1.22474487139f;
inline Vec GridPlane(Vec render) {
    return {render.x-render.y*GroundShear,0,render.z-render.y*GroundShear};
}
inline Vec RenderGround(Vec grid,float height) {
    return {grid.x+height*GroundShear,height,grid.z+height*GroundShear};
}
inline bool Finite(Vec v) {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}
// Compare the current server with the grid position of the SAVED rendered
// pose. Never derive an offset from the current player: that could move an
// old swing to the far side of a teleport. Keep the original 0.65 m limit.
inline bool MatchesPlayer(Vec serverGridWorld,Vec renderedPlayerMetres,float unitsPerMetre) {
    if(!Finite(serverGridWorld)||!Finite(renderedPlayerMetres)||
       !std::isfinite(unitsPerMetre)||unitsPerMetre<1||unitsPerMetre>12)return false;
    const auto saved=GridPlane(renderedPlayerMetres);
    return std::hypot(serverGridWorld.x/unitsPerMetre-saved.x,
                      serverGridWorld.z/unitsPerMetre-saved.z)<=.65f;
}
}
