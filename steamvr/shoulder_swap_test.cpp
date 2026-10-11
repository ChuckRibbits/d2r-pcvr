#include "shoulder_swap.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr;
static void Check(bool b,const char* label){if(!b){std::fprintf(stderr,"FAIL: %s\n",label);std::exit(1);}}
struct Sim {
    shoulder::Gesture g;shoulder::Input in;shoulder::Output out;
    Sim(){in.time=1;in.session=7;in.epoch=1;in.active=in.tracked=in.allowed=true;in.head={0,1.7f,0};in.hand={.23f,1.5f,.14f};}
    shoulder::Output Step(int n=1){while(n--){in.time+=.02;out=g.Step(in);}return out;}
};
int main(){
    for(int hand=0;hand<2;++hand)for(float side:{-.23f,.23f})for(float yaw:{-2.f,0.f,2.f}){
        Sim s;s.in.dominant=hand;s.in.heading=math::Yaw(yaw);s.in.head={2,1.2f,-4};
        s.in.hand=s.in.head+math::Rotate(s.in.heading,{side,-.2f,.14f});
        s.Step(10);s.in.trigger=1;
        Check(s.Step().swap&&s.out.block,"either hand can swap from either shoulder at any yaw, standing or seated");
        for(int i=0;i<15;++i)Check(!s.Step().swap&&s.out.block,"held trigger never repeats or attacks");
        s.in.hand=s.in.head+math::Rotate(s.in.heading,{0,-.3f,-.4f});Check(s.Step().block,"moving away while held still consumes trigger");
        s.in.trigger=0;Check(!s.Step().block,"release restores combat away from shoulder");
    }
    for(math::Vec p:{math::Vec{.23f,1.5f,-.14f},math::Vec{.23f,.9f,0},math::Vec{0,1.5f,.14f},math::Vec{.9f,1.5f,.14f}}){
        Sim s;s.in.hand=p;s.Step(10);s.in.trigger=1;Check(!s.Step().swap&&!s.out.block,"face, hip, head centre and distant hand do not claim trigger");}
    {Sim s;s.in.trigger=1;s.Step(10);Check(!s.out.swap&&s.out.block,"held trigger on tracking start cannot swap");
     s.in.trigger=0;s.Step();s.in.trigger=1;Check(s.Step().swap,"fresh press after release swaps");}
    {Sim s;s.in.hand.z=-.2f;s.Step(10);s.in.trigger=1;s.Step();s.in.hand.z=.14f;
     for(int i=0;i<15;++i)Check(!s.Step().swap,"entering shoulder with trigger held cannot swap");}
    {Sim s;s.Step(10);s.in.allowed=false;s.in.trigger=1;Check(!s.Step().swap,"wheel, grabbed bottle and pending actions prevent swap");
     s.in.allowed=true;Check(!s.Step().swap,"context reopening cannot replay a held press");}
    {Sim s;s.Step(10);s.in.tracked=false;s.in.trigger=1;s.Step();s.in.tracked=true;
     for(int i=0;i<12;++i)Check(!s.Step().swap,"tracking return cannot swap with held trigger");}
    for(int change=0;change<3;++change){Sim s;s.Step(10);s.in.trigger=1;
        if(change==0)++s.in.epoch;if(change==1)++s.in.session;if(change==2)s.in.dominant=1;
        Check(!s.Step().swap,"recenter, session or dominant hand change consumes holds");}
    {Sim s;s.Step(10);s.in.time+=1;s.in.trigger=1;Check(!s.Step().swap,"frame hitch cannot replay gesture");}
    std::puts("Shoulder swap: either shoulder, dominant hand, yaw, seated pose, one press, release and tracking/context guards passed");
}
