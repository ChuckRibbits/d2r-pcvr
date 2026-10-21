#pragma once
#include "frame_math.h"
#include <algorithm>
#include <cmath>

// Minimap zoom. The wrist / palm map and the wheel's MAP tab are one 512x512
// image of the game's corner map, centered on the hero, so they share one zoom.
// While you look at the wrist or palm map, the right stick's up / down is the
// map's: up zooms in, down zooms out. Left / right still turns. Looking away,
// or the map going away, gives the stick back the same frame.
// Jeff, 2026-10-21: "the map zoom in and out should not be buttons. When
// looking at the map, the right stick up and down zooms in and out."
// The zoom lasts while the game runs and is not saved: "Restarting should
// reset the map to default levels in case people forget how to zoom in and out."
namespace framevr::mapzoom {
// 1 is the whole native map in the frame, as before: the source is the game's
// own corner map, so there is nothing further out to zoom to. Pulling down
// until it stops is the way home.
constexpr float Min=1,Max=4;
constexpr float Doublings=1.5f;   // per second at full stick: 1x to 4x in 1.3 s
constexpr float Dead=.3f;         // stick travel ignored, either way
constexpr float Notch=.25f;       // a light tick each quarter crossed; a firmer one at either stop
// Looking at the map: its center within LookIn degrees of the head's forward
// starts it, and it lasts until past LookOut, so a glance at the map's edge
// does not flicker the stick between map and game.
constexpr float LookIn=22,LookOut=30;
inline float Clamp(float z){return std::isfinite(z)?std::clamp(z,Min,Max):Min;}
inline int NotchOf(float z){return int(std::floor((Clamp(z)-Min)/Notch+1e-4f));}
// head: the headset pose; panel: the shown map's center, in the same space.
inline bool Looking(math::Vec headP,math::Quat headQ,math::Vec panel,bool was) {
    const auto toward=panel-headP;
    const float distance2=math::Dot(toward,toward);
    if(!std::isfinite(distance2)||distance2<1e-4f)return false;
    const float cosine=math::Dot(math::Rotate(headQ,{0,0,-1}),toward*(1/std::sqrt(distance2)));
    return std::isfinite(cosine)&&cosine>std::cos((was?LookOut:LookIn)*.0174532925f);
}
struct Input {
    double time=0;
    bool looking=false;   // the wrist / palm map is shown and looked at (Looking)
    float stick=0;        // right stick: up +1, down -1
};
struct Output {
    bool owned=false;     // the right stick's up / down is the map's this frame
    bool zooming=false;   // the stick is pushed past the dead zone
    bool started=false,stopped=false;   // a zoom gesture's edges, for the log
    float zoom=Min;
    int tick=0;           // 1 a quarter crossed, 2 a stop reached
};
class Gesture {
    bool zooming=false;
    double last=0;
    float zoom=Min;
public:
    // Tracking or focus lost: no gesture continues. The zoom stays until the game closes.
    void Reset(){zooming=false;last=0;}
    float Zoom()const{return zoom;}
    Output Step(const Input& in) {
        Output out;
        const double dt=last>0&&in.time>last?std::min(in.time-last,.1):0;
        last=in.time;
        const float y=in.looking&&std::isfinite(in.stick)?std::clamp(in.stick,-1.f,1.f):0;
        const bool now=std::fabs(y)>=Dead;
        out.started=now&&!zooming;out.stopped=!now&&zooming;zooming=now;
        if(now&&dt>0) {
            const float before=zoom;
            zoom=Clamp(zoom*std::exp2(std::copysign((std::fabs(y)-Dead)/(1-Dead),y)*Doublings*float(dt)));
            if(zoom!=before)out.tick=zoom==Min||zoom==Max?2:NotchOf(zoom)!=NotchOf(before)?1:0;
        }
        out.owned=in.looking;out.zooming=zooming;out.zoom=zoom;
        return out;
    }
};
}
