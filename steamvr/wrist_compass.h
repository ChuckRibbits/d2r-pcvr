#pragma once
#include "frame_math.h"
#include <cstdint>
#include <vector>

namespace framevr::compass {
constexpr float Tau=6.28318530718f;
constexpr int Period=1024,Window=512,StripHeight=96,MarkerWidth=32,MarkerHeight=16;
// Below the marker: static button tiles for the map buttons beside the
// wrist map (wristbuttons), one tile per state - the view (Isometric / Top-down)
// and, top-down only, North up / Rotate map. The shown state picks the tile;
// nothing is repainted.
constexpr int ButtonTileWidth=192,ButtonTileHeight=96,ButtonTiles=4,ButtonsY=StripHeight+MarkerHeight;
enum ButtonTile {Isometric,TopDown,NorthUp,RotateMap};
constexpr int Width=Period+Window,Height=StripHeight+MarkerHeight+ButtonTileHeight;
static_assert(ButtonTileWidth*ButtonTiles<=Width,"button tiles fit the compass atlas");
// Two overlapping revolutions let an XR sub-image scroll across north without
// wrapping a texture, repainting text or allocating/uploading every frame.
inline int Crop(float bearing) {
    if(!std::isfinite(bearing))return 0;
    float wrapped=std::fmod(bearing,Tau);if(wrapped<0)wrapped+=Tau;
    return int(std::lround(wrapped*Period/Tau))%Period;
}
class Heading {
    uint64_t session=0;
    float roomYaw=0;
    bool known=false;
public:
    float bearing=0;
    // Map north is screen-up in the native, unmodified map/camera frame.
    // Use exactly the same stage-to-world rotation as the stereo camera.
    bool Update(uint64_t player,bool tracked,math::Quat head,float worldBase,float mapNorth) {
        if(player!=session){session=player;known=false;}
        const float norm=head.x*head.x+head.y*head.y+head.z*head.z+head.w*head.w;
        if(!player||!tracked||!std::isfinite(norm)||norm<.9f||norm>1.1f||
           !std::isfinite(worldBase)||!std::isfinite(mapNorth)){known=false;return false;}
        const auto forward=math::Rotate(head,{0,0,-1});
        // Looking straight down at a watch must not flip the compass. Keep
        // the last horizontal room heading, while still applying stick turns.
        if(forward.x*forward.x+forward.z*forward.z>.04f) {
            roomYaw=std::atan2(-forward.x,-forward.z);known=true;
        }
        if(known)bearing=std::remainder(mapNorth-worldBase-roomYaw,Tau);
        return known;
    }
    void Reset(){*this={};}
};
std::vector<uint8_t> Paint();
}
