#pragma once
#include "journal.h"
#include "native_panels.h"

namespace framevr::dialogue {
// NpcDialogPanel has an absolute root position (no journal-style anchor).
// HD UI fits its 3840x2160 design area inside the source; a square source
// retains the top-aligned widescreen layout. Do not copy any game assets.
inline journal::Rect Crop(journal::Rect native,int width,int height) {
    if(width<=0||height<=0||native.w<=0||native.h<=0||native.x<0||native.y<0||
       native.x>8192||native.y>8192||native.w>3840||native.h>2160)return {};
    const float unit=std::min(width/3840.f,height/2160.f);
    constexpr int padding=8; // retain the border's outer pixels
    const int x=std::clamp(int(std::floor((native.x-padding)*unit)),0,width);
    const int y=std::clamp(int(std::floor((native.y-padding)*unit)),0,height);
    const int right=std::clamp(int(std::ceil((native.x+native.w+padding)*unit)),0,width);
    const int bottom=std::clamp(int(std::ceil((native.y+native.h+padding)*unit)),0,height);
    return {x,y,std::max(0,right-x),std::max(0,bottom-y)};
}
inline panels::Size Size(journal::Rect crop,unsigned width,unsigned height) {
    if(!width||!height||crop.w<=0||crop.h<=0)return {};
    const float metresPerPixel=4.f*panels::Fit(width,height).width/width;
    return {crop.w*metresPerPixel,crop.h*metresPerPixel};
}
}
