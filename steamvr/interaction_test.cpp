#include "interactions.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace framevr::interaction;
static void Check(bool b,const char* s){if(!b){std::fprintf(stderr,"FAIL: %s\n",s);std::exit(1);}}
struct Sim {
    Engine e;Input in;Output out;
    Sim(){in.active=true;in.tracked[0]=in.tracked[1]=true;in.session=10;in.epoch=1;in.head.p={0,1.7f,0};
        in.hand[0].p={.25f,1.3f,-.4f};in.hand[1].p={-.25f,1.3f,-.4f};
        in.snapshot.session=10;in.snapshot.equipmentKnown=in.snapshot.beltKnown=true;in.snapshot.activeSet=1;
        auto& b=in.snapshot.belt[0];b.known=true;b.count=2;b.revision=4;b.items[0]={42,1,Code('h','p','1')};b.items[1]={43,2,Code('m','p','1')};
        Step(3);}
    Output Step(int count=1){while(count--){in.time+=.02;in.at=1000000+uint64_t(in.time*1000);in.snapshot.at=in.at;out=e.Step(in);}return out;}
};
int main(){
    for(int i=0;i<8;++i){float a=i*Pi/4;Check(WheelCell(.5f+std::sin(a)*.28f,.45f-std::cos(a)*.28f)==i,"wedge centers agree");}
    Check(WheelCell(.5f,.45f)==8&&WheelCell(.5f,.96f)==9,"weapon center and utility tab");
    Check(WheelCell(.85f,.96f)==10,"dedicated VR settings entry");
    Check(WheelCell(.5f,.57f)==-1,"dead zone cannot select");
    Check(PotionType(Code('h','p','2'))==Potion::Health&&PotionType(Code('m','p','4'))==Potion::Mana,"potion categories");
    {
        Sim s;s.in.wheel=true;s.in.hover=10;s.in.trigger=1;
        Check(s.Step().settings&&s.out.block&&!s.out.wheel&&s.out.request.action==Action::None,"settings takes ownership without a native command");
        Check(!s.Step(4).settings&&!s.out.wheel&&s.out.block,"held settings selection cannot repeat or cast");
        s.in.active=false;s.Step();s.in.active=true;
        Check(!s.Step().settings&&s.out.block,"return from settings requires trigger release");
    }
    {
        Sim s;s.in.wheel=true;s.in.hover=3;s.in.trigger=1;
        Check(s.Step().select==3&&s.out.block&&!s.out.wheel,"select closes wheel and consumes trigger");
        Check(s.Step(4).select==-2&&!s.out.wheel&&s.out.block,"held Y cannot reopen or cast");
        s.in.trigger=0;Check(!s.Step().block,"trigger release rearms casting");
        s.in.wheel=false;s.Step();s.in.wheel=true;Check(s.Step().wheel,"Y release permits new wheel");
    }
    {
        Sim s;s.in.wheel=true;s.in.hover=9;s.in.trigger=1;Check(s.Step().utility,"utility tab");
        s.in.trigger=0;s.Step();s.in.hover=1;s.in.trigger=1;
        const auto r=s.Step().request;Check(r.action==Action::Equip&&r.index==2,"desired set command");
        s.in.trigger=0;s.in.wheel=false;Check(s.Step().block,"swap blocks until confirmed");
        s.in.reply={r.sequence,10,Result::Applied};Check(s.Step().result==Result::Applied,"confirmed swap feedback");
    }
    {
        Sim s;s.in.hand[0].p=s.out.belt[0].p;s.Step();s.in.grip[0]=1;
        Check(s.Step().bottle==0&&s.out.request.action==Action::None,"physical bottle grab is non-consuming");
        s.in.hand[0].p={0,1.6f,-.08f};s.in.hand[0].q={0,0,.70710678f,.70710678f};
        int requests=0;Request request;
        for(int i=0;i<30;++i){auto o=s.Step();if(o.request.action==Action::Potion){++requests;request=o.request;}Check(o.block,"sip owns combat input");}
        Check(requests==1&&request.item.handle==42,"one drink of exact grabbed item");
        s.in.snapshot.belt[0].items[0]={43,2,Code('m','p','1')};s.in.snapshot.belt[0].items[1]={};++s.in.snapshot.belt[0].revision;
        s.in.reply={request.sequence,10,Result::Applied};
        Check(s.Step().bottleItem==request.item&&s.out.bottleEmpty,"confirmed consumption keeps grabbed identity across mixed-column refill");
        s.in.trigger=1;s.in.grip[0]=0;Check(s.Step().block,"held trigger after bottle drop stays blocked");
        auto snap=s.in.snapshot;snap.belt[0].items[0]={43,2,Code('m','p','1')};snap.belt[0].items[1]={};
        Check(PotionGone(snap,request),"authoritative removal can acknowledge native use");
    }
    {
        Sim s;s.in.hand[0].p=s.out.belt[0].p;s.Step();s.in.grip[0]=1;s.Step();
        ++s.in.snapshot.belt[0].revision;s.in.hand[0].p={0,1.6f,-.08f};s.in.hand[0].q={0,0,1,0};
        for(int i=0;i<30;++i)Check(s.Step().request.action==Action::None&&s.out.bottle<0,"refill/reorder cancels captured bottle");
    }
    {
        Sim s;s.in.hand[0].p=s.out.holster[1].p;s.Step();s.in.grip[0]=1;
        Check(s.Step().block&&s.out.request.action==Action::None,"holster requires deliberate pull");
        s.in.hand[0].p.z-=.15f;
        Check(s.Step().request.action==Action::Equip&&s.out.request.index==2,"holster pulls exact set");
        for(int i=0;i<10;++i)Check(s.Step().request.action==Action::None,"held holster does not spam toggles");
    }
    {
        Sim s;s.in.hand[0].p=s.out.belt[0].p;s.Step();s.in.grip[0]=1;s.Step();s.in.active=false;s.Step();
        s.in.active=true;s.in.hand[0].p={0,1.6f,-.08f};s.in.hand[0].q={0,0,1,0};
        for(int i=0;i<30;++i)Check(s.Step().request.action==Action::None,"tracking/menu return cannot consume held object");
    }
    {
        Sim s;s.in.wheel=true;s.in.hover=9;s.in.trigger=1;s.Step();s.in.trigger=0;s.Step();s.in.hover=4;s.in.trigger=1;
        Check(s.Step().request.action==Action::Potion,"native time and pose time can have unrelated epochs");
        ++s.in.epoch;Check(s.Step().cancelPending,"recenter invalidates queued native transaction");
        for(int i=0;i<12;++i)Check(s.Step().request.action==Action::None,"held selection cannot replay after recenter");
    }
    {
        Sim s;s.in.snapshot.vitalsKnown=true;s.in.snapshot.health=12;s.in.snapshot.maxHealth=100;
        Check(s.Step().critical&&s.out.feedbackHand==1,"critical health cue requests left-hand feedback");
        Check(!s.Step(90).critical,"critical cue is brief, not permanent HUD");
        s.in.snapshot.health=90;Check(!s.Step().critical,"healthy state removes cue");
    }
    {
        Sim s;s.in.hand[0].p=s.out.belt[0].p;s.Step();s.in.grip[0]=1;s.Step();
        s.in.hand[0].p.x=std::numeric_limits<float>::quiet_NaN();
        Check(s.Step().cancelPending&&s.out.bottle<0,"non-finite pose discards a held bottle and pending native work");
        s.in.hand[0].p={0,1.6f,-.08f};s.in.hand[0].q={0,0,1,0};
        for(int i=0;i<30;++i)Check(s.Step().request.action==Action::None,"tracking recovery with grip held cannot drink");
    }
    std::puts("Interaction ownership, wheel, holster, item identity and confirmation tests passed");
}
