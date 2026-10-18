#include "wrist_buttons.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr;
using namespace framevr::wristbuttons;
static void Check(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main() {
    const math::Quat facing{0,0,0,1};
    math::Vec p{};math::Quat q{};
    Place({0,1,0},facing,.24f,View,p,q);
    math::Vec r{};math::Quat rq{};Place({0,1,0},facing,.24f,Rotate,r,rq);
    Check(r.y<1.f&&p.y>1.f&&std::fabs(r.x-p.x)<1e-5f,"view above, rotate below, in one column");
    Check(Shown(View,false)&&!Shown(Rotate,false)&&Shown(Rotate,true),"rotate only on the top-down map");
    Check(p.x>.12f,"the buttons sit right of the map");
    Check(Inside(p,p,q)&&Inside({p.x,p.y,p.z+.04f},p,q),"the tip on the face, or in front of it, counts");
    Check(Inside({p.x+Width*.5f+.01f,p.y,p.z},p,q),"just past the art's edge still counts");
    Check(!Inside({p.x,p.y,p.z+.07f},p,q)&&!Inside({p.x+Width,p.y,p.z},p,q),"a tip away from the face or beside the button does not");
    Check(Hover({p.x,p.y,p.z+.1f},p,q)&&!Inside({p.x,p.y,p.z+.1f},p,q),"a tip hovering in front lights the button without pressing it");
    Check(!Hover({p.x,p.y,p.z+.2f},p,q)&&!Hover({p.x+Width,p.y,p.z+.05f},p,q),"farther off or beside it, no hover");
    const math::Quat turned=math::Yaw(1.2f);Place({0,1,0},turned,.24f,View,p,q);
    Check(Inside(p,p,q),"the box turns with the wrist");
    Poke poke;
    Check(poke.Step(View,true,1000),"entering the button presses it");
    Check(!poke.Step(View,true,1100)&&!poke.Step(View,true,2000),"staying inside does not repeat");
    Check(!poke.Step(View,false,2010),"leaving is not a press");
    Check(poke.Step(View,true,2100),"a new poke presses again");
    Check(!poke.Step(View,false,2120)&&!poke.Step(View,true,2150),"jitter at the edge within RepeatMs is one press");
    std::puts("wrist button tests passed");
}
