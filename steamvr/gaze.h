#pragma once
#include "math.h"
#include <array>
#include <cstdint>

namespace framevr::gaze {
struct UV {float x=.5f,y=.5f;};
// A combined gaze pose supplies a direction, not a measured fixation depth.
// Project its angular direction through each *rendered* eye frustum. A broad
// protected region covers near-object parallax; camera poses never change.
inline bool Project(math::Quat gaze,math::Quat eye,math::Fov f,UV& out) {
    const auto unit=[](math::Quat q){const float n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;return std::isfinite(n)&&std::abs(n-1)<.02f;};
    if(!unit(gaze)||!unit(eye))return false;
    const auto d=math::Rotate(math::Conjugate(eye),math::Rotate(gaze,{0,0,-1}));
    const float l=std::tan(f.left),r=std::tan(f.right),b=std::tan(f.down),t=std::tan(f.up);
    if(d.z>=-1e-4f||!std::isfinite(l+r+b+t)||r<=l||t<=b)return false;
    out={(d.x/-d.z-l)/(r-l),(t-d.y/-d.z)/(t-b)};
    return std::isfinite(out.x)&&std::isfinite(out.y)&&out.x>=0&&out.x<=1&&out.y>=0&&out.y<=1;
}
inline bool Fresh(int64_t display,int64_t sample) {
    // Zero means the runtime cannot supply a timestamp. Do not claim fresh
    // tracking in that case, or reuse stale coordinates during tracking loss.
    return display>0&&sample>0&&sample>=display-100'000'000&&sample<=display+20'000'000;
}
class Stability {
    std::array<UV,2> previous{};
    bool havePrevious=false;
public:
    void Reset(){havePrevious=false;}
    bool Update(bool valid,const std::array<UV,2>& points) {
        if(!valid){Reset();return false;}
        bool stable=havePrevious;
        for(unsigned i=0;i<2;++i)
            stable&=std::hypot(points[i].x-previous[i].x,points[i].y-previous[i].y)<=.12f;
        previous=points;havePrevious=true;
        // Full quality on recovery and large gaze jumps, then follow immediately.
        // Smoothing would add gaze lag and risk shading the new fixation coarsely.
        return stable;
    }
};
}
