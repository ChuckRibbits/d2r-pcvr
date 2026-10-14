#pragma once
namespace framevr::nativehud {
struct Policy {
    int hide=0;
    bool copyToolbar=false,redrawToolbar=false;
    // Classic views only remove a bar when its replacement copy succeeded.
    bool ClearToolbar(bool copied)const{return hide==1&&(!redrawToolbar||copied);}
};
inline bool Gameplay(bool stereo,bool inWorld,bool nativeMenu,bool spatialMenu) {
    return stereo&&inWorld&&!nativeMenu&&!spatialMenu;
}
inline Policy Resolve(bool vrGameplay,bool menu,bool classic,bool pictureReady,bool barChanged,int configuredHide) {
    // The tracked vitals/belt replace the desktop toolbar in native OpenXR.
    // Ignore legacy INI hide=0 and classic-camera redraw preferences here.
    if(vrGameplay)return {1,false,false};
    if(menu)return {};
    if(classic)return pictureReady&&barChanged?Policy{1,true,true}:Policy{};
    return {configuredHide,configuredHide>=1,false};
}
}
