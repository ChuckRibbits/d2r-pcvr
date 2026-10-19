#pragma once
#include "vr_settings.h"
#include <algorithm>
#include <cmath>

// Minimap zoom. The wrist / palm map and the wheel's MAP tab are one 512x512
// image of the game's corner map, centred on the hero, so they share one zoom.
// While a map is shown, a grip pressed on the hand that shows it (the left
// hand for wrist and palm, the wheel's hand for its MAP tab) takes that hand's
// stick: forward zooms in, back zooms out. The stick goes back to walking or
// turning the frame the grip opens or the map goes away.
// Jeff, 2026-10-19: "Zoom in and out on the map when looking at the map. Maybe
// grip button and joystick does zoom."
namespace framevr::mapzoom {
// 1 is the whole native map in the frame, as before: the source is the game's
// own corner map, so there is nothing further out to zoom to. Pulling back
// until it stops is the way home.
constexpr float Min=settings::Definitions[settings::MapZoom].min,Max=settings::Definitions[settings::MapZoom].max;
constexpr float Doublings=1.5f;   // per second at full stick: 1x to 4x in 1.3 s
constexpr float Grip=.6f;         // a held grip, as the interaction engine reads it
constexpr float Dead=.3f;         // stick travel ignored, either way
constexpr float Notch=.25f;       // a light tick each quarter crossed; a firmer one at either stop
constexpr double SaveWait=5;      // a save not seen back by then failed: the stored zoom leads again
inline float Clamp(float z){return std::isfinite(z)?std::clamp(z,Min,Max):Min;}
inline int NotchOf(float z){return int(std::floor((Clamp(z)-Min)/Notch+1e-4f));}
struct Input {
    double time=0;
    bool enabled=false;
    int hand=-1;          // the hand showing the map now drawn (0 right, 1 left); -1 none
    bool busy=false;      // that hand holds a bottle or holster, or is casting: its grip is not ours
    float grip[2]{},stick[2]{};   // stick: forward +1, back -1
    float saved=Min;      // [interactions] minimap_zoom
};
struct Output {
    bool engaged=false;   // the stick of `hand` is the zoom's this frame
    int hand=-1;
    float zoom=Min;
    int tick=0;           // 1 a quarter crossed, 2 a stop reached
    bool save=false;      // a changed zoom waits to be saved
};
class Gesture {
    bool gripWas[2]{},engaged=false,centred=false,unsaved=false,submitted=false;
    int hand=-1;
    double last=0,submittedAt=0;
    float zoom=Min;
public:
    // Tracking or focus lost: a held grip must be released and pressed again. The zoom stays.
    void Reset(){gripWas[0]=gripWas[1]=true;engaged=centred=false;hand=-1;last=0;}
    void Submitted(double time){submitted=true;submittedAt=time;}
    float Zoom()const{return zoom;}
    Output Step(const Input& in) {
        Output out;
        const bool grips[2]={in.grip[0]>Grip,in.grip[1]>Grip};
        const bool pressed[2]={grips[0]&&!gripWas[0],grips[1]&&!gripWas[1]};
        gripWas[0]=grips[0];gripWas[1]=grips[1];
        const double dt=last>0&&in.time>last?std::min(in.time-last,.1):0;
        last=in.time;
        const float saved=Clamp(in.saved);
        if(unsaved&&!engaged&&(std::fabs(saved-zoom)<1e-4f||(submitted&&in.time-submittedAt>SaveWait)))unsaved=submitted=false;
        if(!engaged&&!unsaved)zoom=saved;   // an INI edit or another copy's save shows too
        if(engaged&&(!in.enabled||in.hand!=hand||!grips[hand]))engaged=false;
        // Only a grip pressed while the map shows: a bottle held in that hand, or
        // a grip already down for something else, never turns into a zoom.
        if(!engaged&&in.enabled&&in.hand>=0&&in.hand<2&&!in.busy&&pressed[in.hand]){engaged=true;hand=in.hand;centred=false;}
        if(engaged) {
            const float y=std::isfinite(in.stick[hand])?std::clamp(in.stick[hand],-1.f,1.f):0;
            // A stick already pushed (walking) when the grip closed waits for the centre first.
            if(std::fabs(y)<Dead)centred=true;
            else if(centred&&dt>0) {
                const float before=zoom;
                zoom=Clamp(zoom*std::exp2(std::copysign((std::fabs(y)-Dead)/(1-Dead),y)*Doublings*float(dt)));
                if(zoom!=before) {
                    unsaved=true;submitted=false;
                    out.tick=zoom==Min||zoom==Max?2:NotchOf(zoom)!=NotchOf(before)?1:0;
                }
            }
        }
        out.engaged=engaged;out.hand=engaged?hand:-1;out.zoom=zoom;out.save=unsaved&&!engaged&&!submitted;
        return out;
    }
};
// The zoom saved like any VR setting: one changed key on the current values.
inline settings::Request SaveRequest(const settings::Snapshot& s,float zoom) {
    if(!s.ready||s.result==settings::Result::Saving)return {};
    auto values=s.values;values[settings::MapZoom]=Clamp(zoom);
    return {s.values,values,settings::Changed(s.values,values),s.sequence+1};
}
}
