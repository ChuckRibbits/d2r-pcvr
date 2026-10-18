#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <mutex>

// Landing telegraphs for lobbed skills (Fire Blast): a ground circle at the
// target the server actually gave each missile, shown from the moment it is
// thrown. Its fill closes from the rim inward over the expected flight and it
// fades out when that bomb bursts. Several bombs can be in the air at once.
// The game thread records; the render thread samples. Positions are native
// grid cells; no pointers are kept.
namespace framevr::telegraph {
inline constexpr float CellsPerSecond=14.f;   // measured: 31 cells in 2.4 s, 22 in 1.6 s
inline constexpr float RadiusCells=2.5f;      // approximate burst area
inline constexpr float MatchCells=12.f;       // a burst farther than this from every circle ends the oldest
inline constexpr uint64_t MinFlightMs=250,LateMs=1500,FadeMs=350;
inline constexpr int MaxMarks=6;              // D2R_DepthFog.fx draws RingPos0..5
struct Mark {
    float x=0,y=0;            // target cell
    uint64_t cast=0,flight=0; // ms; flight is the expected time to land
    uint64_t landed=0;        // ms; 0 until the burst is seen
    uint32_t sequence=0;
    int element=0;            // spellaim::Element: the circle's colour
    float radius=RadiusCells; // cells; for a strip, its half-width
    float dirX=0,dirY=0;      // a strip's direction on the grid (unit)
    float halfLength=0;       // cells; 0 = a circle
};
// Each ground skill's area and time, measured from the missiles the game
// created (2026-10-18, 5 casts each): Blizzard's shards land up to ~7 cells
// out for 4.1 s; Meteor's 18 fire patches within 5 cells at 2.65 s; Fire Wall's
// flames on a 10-cell line square to the cast; Fissure's vents within ~7 cells
// for 3.5 s. Hydra and traps are placed objects: a small spot.
struct Area {uint64_t showMs;float radius,halfLength;};
inline Area ForSkill(int skill) {
    switch(skill) {
        case 59: return {4100,7.f,0};      // Blizzard
        case 56: return {2650,5.f,0};      // Meteor: fills until it lands
        case 51: return {700,.75f,5.f};    // Fire Wall: a strip across the cast
        case 234: return {3500,7.f,0};     // Fissure
        case 244: return {900,2.5f,0};     // Volcano (not measured yet)
        default: return {700,2.f,0};       // traps, Shock Web, Hydra
    }
}
struct Draw {bool on=false;float fill=0,alpha=0;bool burst=false;};
using Marks=std::array<Mark,MaxMarks>;
inline uint64_t ExpectedFlight(float originX,float originY,float targetX,float targetY) {
    const float cells=std::hypot(targetX-originX,targetY-originY);
    if(!std::isfinite(cells))return MinFlightMs;
    return std::max<uint64_t>(MinFlightMs,uint64_t(cells/CellsPerSecond*1000.f));
}
// Fill grows 0 -> 1 until the expected landing; the burst (or a missed burst
// event, LateMs after the expected landing) fades the circle out.
inline Draw Sample(const Mark& m,uint64_t now) {
    Draw d;
    if(!m.cast||now<m.cast)return d;
    const uint64_t end=m.landed?m.landed:m.cast+m.flight+LateMs;
    if(now>=end+FadeMs)return d;
    d.on=true;
    d.fill=m.flight?std::clamp(float(now-m.cast)/float(m.flight),0.f,1.f):1.f;
    if(now>=end){d.burst=m.landed!=0;d.fill=1;d.alpha=1.f-float(now-end)/float(FadeMs);}
    else d.alpha=1;
    return d;
}
inline bool AnyOn(const Marks& marks,uint64_t now) {
    for(const auto& m:marks)if(Sample(m,now).on)return true;
    return false;
}
class Board {
    std::mutex mutex;
    Marks marks{};
    uint32_t sequence=0;
public:
    // A new bomb takes a finished slot, else the oldest one.
    void Cast(float originX,float originY,float targetX,float targetY,uint64_t now) {
        std::lock_guard lock(mutex);
        Mark* slot=&marks[0];
        for(auto& m:marks) {
            if(!Sample(m,now).on){slot=&m;break;}
            if(m.cast<slot->cast)slot=&m;
        }
        *slot={targetX,targetY,now,ExpectedFlight(originX,originY,targetX,targetY),0,++sequence,0,RadiusCells};
    }
    // A skill cast at a ground point with no missile to follow (traps,
    // Blizzard, Meteor...): its circle fills over showMs and then fades. A
    // repeat at nearly the same point within RepeatMs is the same cast.
    static constexpr uint64_t RepeatMs=350;
    bool Place(float x,float y,uint64_t now,uint64_t showMs,int element,float radius,float dirX=0,float dirY=0,float halfLength=0) {
        std::lock_guard lock(mutex);
        for(const auto& m:marks)
            if(m.cast&&now>=m.cast&&now-m.cast<RepeatMs&&std::hypot(m.x-x,m.y-y)<1.5f)return false;
        Mark* slot=&marks[0];
        for(auto& m:marks) {
            if(!Sample(m,now).on){slot=&m;break;}
            if(m.cast<slot->cast)slot=&m;
        }
        showMs=std::max<uint64_t>(showMs,MinFlightMs);
        const float d=std::hypot(dirX,dirY);
        if(!(halfLength>0&&d>1e-4f))dirX=dirY=halfLength=0;else{dirX/=d;dirY/=d;}
        *slot={x,y,now,showMs,now+showMs,++sequence,element,radius,dirX,dirY,halfLength};
        return true;
    }
    // A burst ends the airborne circle nearest to where it came down, or the
    // oldest airborne one when none is near. Returns that circle (cast 0: none).
    Mark Landed(float x,float y,uint64_t now) {
        std::lock_guard lock(mutex);
        Mark* best=nullptr;float bestCells=MatchCells;Mark* oldest=nullptr;
        for(auto& m:marks) {
            if(!m.cast||m.landed||!Sample(m,now).on)continue;
            const float cells=std::hypot(m.x-x,m.y-y);
            if(cells<=bestCells){bestCells=cells;best=&m;}
            if(!oldest||m.cast<oldest->cast)oldest=&m;
        }
        if(!best)best=oldest;
        if(!best)return {};
        const Mark before=*best;
        best->landed=now;
        return before;
    }
    Marks Get() {std::lock_guard lock(mutex);return marks;}
};
inline Board& Current() {static Board board;return board;}
}
