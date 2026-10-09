#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace framevr {
// A bounded immutable bank avoids texture creation/uploads on gaze motion.
// Quantization error is at most half a cell on each axis. Expand the protected
// ellipse by that diagonal in its normalized coordinates (UV distance * 2).
constexpr int GazeGrid=9,GazeMaskCount=GazeGrid*GazeGrid;
constexpr float GazeQuantizationMargin=1.414213563f/(GazeGrid-1);
inline int GazeCell(float x,float y) {
    const auto cell=[](float v){return int(std::round(std::clamp(v,0.f,1.f)*(GazeGrid-1)));};
    return cell(y)*GazeGrid+cell(x);
}
inline float GazeGridCoordinate(int cell){return float(cell)/(GazeGrid-1);}
// Keep every tile touching the protected ellipse at 1x1. Coordinates include
// viewport offsets and asymmetric eye frusta; areas outside the viewport stay
// full quality. The conservative pilot uses only universally supported 2x2.
inline uint8_t FixedRate(uint32_t x,uint32_t y,uint32_t tile,float vx,float vy,
                         float vw,float vh,float cx,float cy,float radius) {
    if(!tile||vw<=0||vh<=0||!std::isfinite(radius))return 0;
    const float left=float(x*tile),top=float(y*tile);
    if(left>=vx+vw||top>=vy+vh||left+tile<=vx||top+tile<=vy)return 0;
    const float centerX=vx+vw*cx,centerY=vy+vh*cy;
    const float dx=std::max({left-centerX,centerX-(left+tile),0.f})/(vw*.5f);
    const float dy=std::max({top-centerY,centerY-(top+tile),0.f})/(vh*.5f);
    return dx*dx+dy*dy<=radius*radius?0:5;
}
}
