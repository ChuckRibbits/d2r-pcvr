#pragma once
#include "journal.h"
#include "native_panels.h"

namespace framevr::dialogue {
// Live NpcDialogPanel bounds are already in source pixels, unlike authored
// layout coordinates. Do not apply HD design scaling a second time: at 4096
// square that moved the crop right and cut off the start of each choice.
inline journal::Rect Crop(journal::Rect native,int width,int height) {
    if(width<=0||height<=0||native.w<=0||native.h<=0||native.x<0||native.y<0||
       native.x>8192||native.y>8192||native.w>3840||native.h>2160)return {};
    constexpr int padding=8; // retain the border's outer pixels
    const int x=std::clamp(native.x-padding,0,width);
    const int y=std::clamp(native.y-padding,0,height);
    const int right=std::clamp(native.x+native.w+padding,0,width);
    const int bottom=std::clamp(native.y+native.h+padding,0,height);
    return {x,y,std::max(0,right-x),std::max(0,bottom-y)};
}
inline panels::Size Size(journal::Rect crop,unsigned width,unsigned height) {
    if(!width||!height||crop.w<=0||crop.h<=0)return {};
    const float metresPerPixel=4.f*panels::Fit(width,height).width/width;
    return {crop.w*metresPerPixel,crop.h*metresPerPixel};
}
}
