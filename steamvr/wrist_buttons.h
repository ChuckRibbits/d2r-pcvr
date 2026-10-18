#pragma once
#include "frame_math.h"
#include <cmath>
#include <cstdint>

// Map buttons beside the wrist (or palm) minimap: the view (Isometric /
// Top-down) and, top-down only, whether the map rotates with you. Pressed with
// the right hand's laser and trigger, or by poking with that controller's tip;
// each press changes the saved setting once. The art is static tiles in
// the compass atlas, one per state (compass::ButtonTile).
namespace framevr::wristbuttons {
constexpr float Width=.075f,Height=.04f,Gap=.008f;
constexpr float Depth=.05f;         // how far in front of or behind the face a tip still counts
constexpr float Margin=.015f;       // the box a little wider than the art: the tip is hard to place exactly
constexpr uint64_t RepeatMs=400;    // one press per poke, even with tracking jitter
enum Button {View,Rotate,Count};

// The rotate button exists only on the top-down map (rotating the slanted map skews it).
inline bool Shown(int button,bool topDown){return button==View||(button==Rotate&&topDown);}
// The button's centre and rotation: a column on the map's right edge, View above Rotate.
inline void Place(math::Vec mapCentre,math::Quat mapRotation,float mapSize,int button,math::Vec& p,math::Quat& q) {
    const float x=mapSize*.5f+Gap+Width*.5f;
    const float y=(button==View?1.f:-1.f)*(Height*.5f+Gap*.5f);
    p=mapCentre+math::Rotate(mapRotation,{x,y,0});q=mapRotation;
}
// The tip in the button's frame (x across, y up, z out of its face).
inline math::Vec Local(math::Vec tip,math::Vec p,math::Quat q) {
    return math::Rotate(math::Conjugate(q),{tip.x-p.x,tip.y-p.y,tip.z-p.z});
}
// The tip inside the button's box (its face plus Margin, Depth either side).
inline bool Inside(math::Vec tip,math::Vec p,math::Quat q,float w=Width,float h=Height) {
    const auto local=Local(tip,p,q);
    return std::isfinite(local.x+local.y+local.z)&&std::fabs(local.x)<=w*.5f+Margin&&std::fabs(local.y)<=h*.5f+Margin&&std::fabs(local.z)<=Depth;
}
// The tip over the button, about to press it: within the box across, up to
// HoverDepth in front of the face. The button is drawn bigger meanwhile.
constexpr float HoverDepth=.12f,HoverScale=1.2f;
inline bool Hover(math::Vec tip,math::Vec p,math::Quat q,float w=Width,float h=Height) {
    const auto local=Local(tip,p,q);
    return std::isfinite(local.x+local.y+local.z)&&std::fabs(local.x)<=w*.5f+Margin&&std::fabs(local.y)<=h*.5f+Margin&&
        local.z>=-Depth&&local.z<=HoverDepth;
}
// A press on entering the box, then nothing until the tip has left it and
// RepeatMs has passed.
class Poke {
    bool inside[Count]{};
    uint64_t at[Count]{};
public:
    bool Step(int button,bool in,uint64_t now) {
        if(button<0||button>=Count)return false;
        const bool press=in&&!inside[button]&&now-at[button]>=RepeatMs;
        if(press)at[button]=now;
        inside[button]=in;
        return press;
    }
    void Reset(){for(auto& b:inside)b=false;}
};
}
