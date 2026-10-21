#pragma once
#include "bow_flight.h"

// Bow path laser: a faint straight line from the drawn arrow's tip along the
// direction a release would fly. Physical arrows have no gravity, so the line
// is the real path. It stops where the flight meets the launch floor; walls
// and monsters are tested only by the flight itself, so the line otherwise
// ends at MaxLength. Room metres throughout; yaw and scale keep it straight.
namespace framevr::bowlaser {
using math::Vec;
inline constexpr float MaxLength=15; // metres, without a wall or monster test
struct Path {Vec start{},end{};float length=0;bool floor=false;};
// Shown only while a release now would shoot: the same full-draw gate and
// forward check as the bow's release.
inline bool Shown(bool charged,Vec direction,Vec bowForward) {
    return charged&&bowflight::Finite(direction)&&math::Dot(direction,bowForward)>.15f;
}
// tail is the nocked arrow's tail (the string hand); floorY the launch floor's
// height in the same space. Length 0 hides the line.
inline Path Trace(Vec tail,Vec direction,bool floorKnown,float floorY) {
    Path p;
    if(!bowflight::Finite(tail)||!bowflight::Finite(direction)||std::fabs(bowflight::Length(direction)-1)>.01f)return p;
    p.start=p.end=tail+direction*bowflight::ArrowLength;p.length=MaxLength;
    if(floorKnown&&std::isfinite(floorY)) {
        const float floor=floorY+bowflight::FloorClearance,above=p.start.y-floor;
        if(above<=0)return {p.start,p.start,0,true};
        if(direction.y<-1e-4f&&above/-direction.y<MaxLength){p.length=above/-direction.y;p.floor=true;}
    }
    p.end=p.start+direction*p.length;return p;
}
}
