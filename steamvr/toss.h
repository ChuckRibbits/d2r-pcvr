#pragma once
#include "interactions.h"
#include "world_coordinates.h"

// Traps and bombs thrown to place (#224): the Assassin's Fire Blast, Shock
// Web, Blade Sentinel, the three sentries and the two wakes. A quick press
// casts where you point and a steady hold casts natively as before; a throw
// places the cast where its arc lands. The throw picks the spot only: the
// game's own cast, reach, damage and placement rules stay.
namespace framevr::toss {
inline bool Skill(int id) {
    switch(id) {
        case 251:case 256:case 257:case 261:case 262:case 271:case 272:case 276:return true;
        default:return false;
    }
}
// The model held while winding up (the potion cache's, from the game's own
// data): Fire Blast's bomb, Shock Web's caltrops, Blade Sentinel's blade; the
// sentries share one model, and the wakes another, as the game shares them.
inline uint32_t Model(int id) {
    switch(id) {
        case 251:return interaction::Code('t','f','b');
        case 256:return interaction::Code('t','s','w');
        case 257:return interaction::Code('t','b','s');
        case 261:case 271:case 276:return interaction::Code('t','s','t');
        case 262:case 272:return interaction::Code('t','w','k');
        default:return 0;
    }
}
// Where a toss released at room point p with room velocity v (metres, y up)
// lands, in grid cells: the stage maps room to world metres, the floor under
// the player at release (world metres) is the landing plane, and cells are
// the unsheared grid at half a cell per rendered unit.
inline bool Landing(const interaction::Stage& stage,float floor,float unitsPerMetre,math::Vec p,math::Vec v,float& x,float& y) {
    if(!stage.known||!std::isfinite(unitsPerMetre)||unitsPerMetre<1||unitsPerMetre>12)return false;
    const auto worldP=stage.World(p),worldV=math::Rotate(stage.base,v);
    const double t=interaction::LandingTime(worldP,worldV,floor);
    if(t<=0)return false;
    const auto landed=interaction::Ballistic(worldP,worldV,float(t));
    const auto grid=coordinates::GridPlane({landed.x,floor,landed.z});
    x=grid.x*unitsPerMetre*.5f;y=grid.z*unitsPerMetre*.5f;
    return std::isfinite(x)&&std::isfinite(y)&&x>=1&&y>=1&&x<=65534&&y<=65534;
}
}
