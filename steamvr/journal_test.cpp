#include "journal.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr;
static void Check(bool value,const char* what){if(!value){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
int main(){
    const journal::Rect mouse{236,-651,1162,1507},controller{-890,-583,1795,1415};
    auto c=journal::Crop(mouse,3840,2160,1);
    Check(c.x==236&&c.y==206&&c.w==1162&&c.h==1507,"mouse quest crop follows HD anchor");
    c=journal::Crop(controller,3840,2160,1);
    Check(c.x==1030&&c.y==274&&c.w==1795&&c.h==1415,"controller quest crop centered");
    math::Vec p{};Check(journal::Pixel(c,.25f,.75f,3840,2160,p),"valid book point");
    Check(std::fabs(p.x*3840-(1030+1795*.25f))<.01f&&std::fabs(p.y*2160-(274+1415*.75f))<.01f,"book UV maps back to original panel");
    auto small=journal::Crop(controller,1920,1080,.8f);
    Check(small.w==718&&small.h==566&&small.x==604,"safe area and resolution scaling");
    Check(journal::Crop(mouse,0,1080,1).w==0&&!journal::Pixel({},.5,.5,1920,1080,p),"invalid crop cannot click world");
    journal::Placement book;interaction::Pose hand{{-.25f,1.2f,-.4f},{0,0,0,1}};
    book.Open();book.Step(hand,true,false);Check(book.Placed()&&book.Held(),"open in hand without grip clutch");
    book.Step(hand,true,true);book.Step(hand,true,false);Check(!book.Held(),"grip release pins");
    auto pinned=book.Get();hand.p.x+=1;book.Step(hand,true,false);book.Step(hand,true,true);
    Check(!book.Held()&&book.Get().p.x==pinned.p.x,"far grip cannot teleport pinned book");
    book.Step(hand,true,false);hand.p=pinned.p;book.Step(hand,true,true);Check(book.Held(),"fresh grip near cover picks book up");
    book.Step(hand,false,true);hand.p.x+=.1f;book.Step(hand,true,true);Check(!book.Held(),"tracking return cannot resume held grab");
    std::puts("Journal crop, pointer coordinates, hand placement, pinning and tracking tests passed");
}
