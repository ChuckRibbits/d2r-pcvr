#include "native_panels.h"
#include "panel_input_mode.h"
#include "journal.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr;
static void Check(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main(){
    for(auto name:{"UISwitcher","VendorPanelLayout","VendorPanelOriginalLayout","TradeLayout","GamblePanel"})
        Check(panels::Modal(name)&&panels::OpaqueBackground(name),"tabbed and trade surfaces use the shared black background");
    for(auto name:{"NpcDialogPanel","ScrollSpeechPanel","QuestLogPanelExpansion","GoldTransferPanel","QuantityPanel","HUDPanel","HUDTradeHint","MiniMenuPanel","TooltipsPanel"})
        Check(!panels::OpaqueBackground(name),"dialogue, journal and auxiliary panels do not independently black out the world");
    panels::InputModeLease mode;
    Check(!mode.PreserveController(100,true),"desktop input untouched without VR ownership");
    mode.Refresh(100);
    Check(mode.PreserveController(100,true),"guard armed before queued cursor movement");
    Check(!mode.PreserveController(101,false),"do not silently force a mouse layout into controller mode");
    Check(mode.PreserveController(399,true),"queued click and release retain controller mode");
    Check(!mode.PreserveController(400,true),"tracking/focus loss expires guard without another frame");
    mode.Refresh(500);mode.Refresh(750);
    Check(mode.PreserveController(1049,true)&&!mode.PreserveController(1050,true),"active pointer renews bounded ownership");
    Check(panels::MessageHash("InputMessage")==0x4ae067c6248b6042ull&&
          panels::MessageHash("LeftMouseButtonDown")==0xbdf8daff2e44666full,"native pointer message hashes");
    for(auto command:{"MouseMove","MouseScroll","LeftMouseButtonDown","LeftMouseButtonUp","RightMouseButtonDown","RightMouseButtonUp"})
        Check(panels::PointerMessage(panels::MessageHash("InputMessage"),panels::MessageHash(command)),"all laser event kinds use pointer widget behavior");
    Check(!panels::PointerMessage(panels::MessageHash("PanelManager"),panels::MessageHash("OpenPanel")),"panel changes use actual controller layout");
    bool pointer=false;int calls=0;
    Check(panels::RoutePointer(true,pointer,[&]{
        ++calls;Check(pointer,"native UI receives pointer behavior");
        {panels::PointerScope nested(pointer,false);Check(!pointer,"nested panel open retains controller mode");}
        Check(pointer,"nested dispatch restores outer pointer behavior");
        return false; // A blank panel pixel: no widget handled this click.
    }),"UI miss is consumed before world input");
    Check(calls==1&&!pointer,"native pointer callback runs once and restores mode");
    Check(panels::RoutePointer(true,pointer,[&]{return true;}),"handled UI event remains consumed");
    Check(!panels::RoutePointer(false,pointer,[&]{Check(!pointer,"desktop event retains native mode");return false;}),"desktop miss is not swallowed");
    Check(panels::RoutePointer(false,pointer,[&]{return true;}),"desktop handled result preserved");
    Check(!panels::RoutePointer(mode.Active(1050),pointer,[&]{Check(!pointer,"expired ownership restores normal dispatch");return false;}),"ownership expiry stops consuming events");
    // Recorded vendor failure: MouseMove finds a Yes/No row, then the separate
    // controller-cursor pass replaces it with the grid beneath the popup.
    enum Hover {None,Yes,No,VendorGrid};
    auto nativeFocus=[&](Hover hit,bool nativeSnap,bool binding=false){
        Hover hover=hit;
        if(nativeSnap&&panels::CursorSnapAllowed(pointer,binding))hover=VendorGrid;
        return hover;
    };
    Check(nativeFocus(Yes,true)==VendorGrid,"fixture reproduces native cursor snap stealing popup focus");
    for(Hover row:{Yes,No}) {
        panels::RoutePointer(true,pointer,[&]{
            Check(nativeFocus(row,true)==row,"laser keeps exact Yes/No row through native focus update");
            Check(nativeFocus(None,true)==None,"laser miss cannot snap to a nearby purchase or grid");
            Check(nativeFocus(row,true,true)==VendorGrid,"explicit controller assignment keeps native snapping");
            {panels::PointerScope nested(pointer,false);
                Check(nativeFocus(row,true)==VendorGrid,"panel layout message keeps native cursor behavior");}
            Check(nativeFocus(row,true)==row,"nested message restores laser focus policy");
            return true;
        });
        Check(nativeFocus(row,true)==VendorGrid,"controller snapping restored after pointer callback");
        Check(nativeFocus(row,false)==row,"native disabled snapping is never enabled");
    }
    for(auto name:{"UISwitcher","ControllerOverlay","PlayerInventoryExpansionLayout","VendorPanelLayout","BankExpansionLayout","NpcDialogPanel","ScrollSpeechPanel","QuestLogSpeechPanel","HoradricCubeLayout","GoldTransferPanel"})
        Check(panels::Modal(name),"native management/dialog root routed as modal");
    for(auto name:{"HUDPanel","HUDMonsterHealth","MiniMenuPanel","TooltipsPanel","ShowItemsPanel","PanelBordersPanel"})
        Check(!panels::Modal(name),"persistent HUD must not open VR workspace");
    panels::MouseGate mouse;
    mouse.Step(false,false,true,false);
    Check(!mouse.Step(true,true,true,false).press[0],"held attack cannot pick a dialog option");
    mouse.Step(true,true,false,false);
    Check(mouse.Step(true,true,true,false).press[0],"fresh trigger begins item drag");
    Check(!mouse.Step(true,false,true,false).release[0]&&mouse.down[0],"ray miss keeps item held");
    Check(mouse.Step(true,false,false,false).release[0],"physical release ends drag at last valid pixel");
    Check(mouse.Step(true,true,false,true).press[1],"A is native context click");
    Check(mouse.Step(false,true,false,true).release[1],"lost focus releases context click");
    Check(!mouse.Step(true,true,false,true).press[1],"focus return requires new context press");
    mouse.Suspend();Check(!mouse.Step(true,true,true,false).press[0],"mode transition consumes held trigger");
    panels::ScrollGate scroll;
    Check(scroll.Step(true,1,0)==1&&scroll.Step(true,1,.2)==0&&scroll.Step(true,1,.4)==1,"scroll delay and repeat");
    Check(scroll.Step(true,-1,.41)==-1&&scroll.Step(false,-1,.6)==0,"scroll direction and focus guard");
    // Source pixels, physical panel and mouse mapping agree at multiple aspects.
    for(auto size:{journal::Rect{0,0,3840,2160},journal::Rect{0,0,2560,1440},journal::Rect{0,0,1920,1200}}) {
        for(float u:{.01f,.25f,.5f,.75f,.99f})for(float v:{.01f,.5f,.99f}) {
            const auto rot=math::Yaw(.65f);const math::Vec origin{.3f,1.4f,-1.25f};
            auto point=origin+math::Rotate(rot,{(u-.5f)*panels::Width,(.5f-v)*panels::Height(size.w,size.h),0});
            const auto front=math::Rotate(rot,{0,0,1});math::PanelHit hit;math::Vec pixel{};
            Check(math::IntersectPanel(point+front,front*-1,origin,rot,panels::Width,panels::Height(size.w,size.h),hit),"rotated panel ray");
            Check(journal::Pixel(size,hit.u,hit.v,size.w,size.h,pixel)&&std::fabs(pixel.x-u)<.00001f&&std::fabs(pixel.y-v)<.00001f,"native pixel mapping across both panes and tooltips");
        }
    }
    std::puts("Native panel routing, drag ownership, focus recovery, scrolling and pixel mapping passed");
}
