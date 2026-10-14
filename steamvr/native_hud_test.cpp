#include "native_hud.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
using namespace framevr::nativehud;
int main(){
    assert(Recent(100,100)&&Recent(100,599)&&!Recent(100,600)&&!Recent(0,1)&&!Recent(100,99));
    VisibilityOverride visibility;bool shown=true;int calls=0;
    auto set=[&](bool visible){shown=visible;++calls;return true;};
    visibility.Update(false,true,set);assert(shown&&calls==0); // desktop untouched
    visibility.Update(true,true,set);assert(!shown&&visibility.hidden);
    shown=true;visibility.Update(true,true,set);assert(!shown); // game rebuilt controller HUD
    visibility.Update(false,true,[](bool){return false;});assert(visibility.hidden); // retry restoration
    visibility.Update(false,true,set);assert(shown&&!visibility.hidden); // menu/VR stop
    visibility.Update(true,false,set);assert(!visibility.hidden); // unloaded panel
    visibility.Update(true,true,set);assert(!shown&&visibility.hidden); // new game
    visibility.Update(false,false,set);assert(!visibility.hidden); // no stale handle on exit
    // Reproduce both reported routes: old hide=0 INIs and a classic camera
    // with its toolbar zoom/position effect enabled. Neither can leak a bar.
    for(int configured:{0,1,2})for(bool classic:{false,true})for(bool changed:{false,true})for(bool staleMenu:{false,true}){
        const auto p=Resolve(true,staleMenu,classic,true,changed,configured);
        assert(p.hide==1&&!p.copyToolbar&&!p.redrawToolbar&&p.ClearToolbar(false));
    }
    assert(Gameplay(true,true,false,false));
    assert(!Gameplay(true,true,true,false)); // inventory/waypoint fallback
    assert(!Gameplay(true,true,true,true));  // spatial menu
    assert(!Gameplay(true,false,false,false)); // title/character selection
    assert(!Gameplay(false,true,false,false)); // stopped/lost XR or desktop
    // Releasing the per-eye override restores the exact legacy behavior.
    auto p=Resolve(false,false,false,false,false,0);
    assert(p.hide==0&&!p.copyToolbar&&!p.ClearToolbar(true));
    p=Resolve(false,false,false,false,false,1);
    assert(p.hide==1&&p.copyToolbar&&!p.redrawToolbar&&p.ClearToolbar(false));
    p=Resolve(false,false,false,false,false,2);assert(p.hide==2&&p.copyToolbar);
    p=Resolve(false,true,false,true,true,2);assert(p.hide==0&&!p.copyToolbar);
    p=Resolve(false,false,true,false,true,1);assert(p.hide==0);
    p=Resolve(false,false,true,true,false,1);assert(p.hide==0);
    p=Resolve(false,false,true,true,true,0);
    assert(p.hide==1&&p.copyToolbar&&p.redrawToolbar&&!p.ClearToolbar(false)&&p.ClearToolbar(true));
    std::puts("Native VR toolbar suppression, menu/desktop preservation and legacy redraw policy passed");
}
