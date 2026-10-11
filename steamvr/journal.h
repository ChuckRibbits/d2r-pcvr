#pragma once
#include "interactions.h"

namespace framevr::journal {
struct Rect {int x=0,y=0,w=0,h=0;};
// Native HD panel rectangles use a 2160-high layout and one of two anchors.
// Texture cropping and pointer mapping must share these exact coordinates.
inline Rect Crop(Rect native,int width,int height,float safeScale) {
    if(width<=0||height<=0||native.w<=0||native.h<=0||!std::isfinite(safeScale)||safeScale<=0||safeScale>1)return {};
    const float unit=height*safeScale/2160.f;
    const float left=width*(.5f*(1-safeScale)+(native.x<0?.5f:0.f)*safeScale)+native.x*unit;
    const float top=height*(.5f*(1-safeScale)+.397f*safeScale)+native.y*unit;
    const int x=std::clamp(int(left),0,width),y=std::clamp(int(top),0,height);
    return {x,y,std::clamp(int(left+native.w*unit),x,width)-x,std::clamp(int(top+native.h*unit),y,height)-y};
}
inline bool Pixel(Rect crop,float u,float v,int width,int height,math::Vec& result) {
    if(crop.w<=0||crop.h<=0||width<=0||height<=0||!std::isfinite(u)||!std::isfinite(v)||u<0||u>1||v<0||v>1)return false;
    result={(crop.x+u*crop.w)/width,(crop.y+v*crop.h)/height,0};return true;
}
class Placement {
    bool held=true,gripSeen=false,gripWas=false,placed=false;
    interaction::Pose pose{};
public:
    void Open(){held=true;gripSeen=gripWas=placed=false;pose={};}
    void LoseTracking(){held=false;gripSeen=true;gripWas=true;}
    bool Held() const{return held;}
    bool Placed() const{return placed;}
    interaction::Pose Get() const{return pose;}
    void Step(interaction::Pose hand,bool tracked,bool grip) {
        if(!tracked||!interaction::Valid(hand)){LoseTracking();return;}
        const auto delta=hand.p-pose.p;
        if(grip&&!gripWas&&(!placed||math::Dot(delta,delta)<.35f*.35f)){held=true;gripSeen=true;}
        if(held){
            pose.q=math::Multiply(hand.q,{-.70710678f,0,0,.70710678f});
            pose.p=hand.p+math::Rotate(pose.q,{0,.17f,.025f});placed=true;
            if(grip)gripSeen=true;
            if(!grip&&gripSeen)held=false;
        }
        gripWas=grip;
    }
};
}
