#include "gameplay.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace framevr;
using namespace framevr::gameplay;
static void Check(bool b,const char* message) {if(!b){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
struct Sim {
    Engine engine;Input in;Output out;
    Sim() {
        in.active=true;in.tracked[0]=in.tracked[1]=in.aimTracked=true;
        in.head.p={0,1.65f,0};in.hand[0].p={.25f,1.25f,-.4f};in.hand[1].p={-.25f,1.25f,-.4f};
        in.aim=in.hand[0];in.weapon=Weapon::Melee;in.equipment=1;
        Step();Step();
    }
    Output Step(int n=1) {while(n--){in.time+=.02;out=engine.Step(in);}return out;}
    void Move(Vec p,int n=10) {
        const Vec start=in.hand[0].p;
        for(int i=1;i<=n;++i){in.hand[0].p=start+(p-start)*(float(i)/n);Step();}
    }
    void Nock() {
        in.weapon=Weapon::Bow;in.hand[0].p={0,1.3f,-.4f};in.hand[1].p={0,1.3f,-.5f};
        in.grip[1]=0;Step();Step();in.trigger=1;
        Check(Step().bowDrawing,"near-hand pinch nocks bow");
    }
};
int main() {
    {
        Sim s;s.in.trigger=1;Check(s.Step().attack,"button attack fallback");
        s.in.active=false;Check(!s.Step().attack,"menu cancels attack");s.in.active=true;
        Check(!s.Step(3).attack,"held trigger cannot attack on menu return");
        s.in.trigger=0;s.Step();s.in.trigger=1;Check(s.Step().attack,"release rearms fallback");
    }
    {
        Sim s;Check(s.Step().meleeContact,"equipped melee permits contacts without any button");
        s.in.grip[0]=1;Check(s.Step().meleeContact&&!s.out.attack,"grip neither arms nor attacks");
        for(int i=0;i<10;++i){s.in.hand[0].p.x+=.04f;Check(!s.Step().attack,"swing never synthesizes A or auto-targets");}
        s.in.trigger=1;Check(s.Step().attack&&!s.out.meleeContact,"explicit fallback cannot double up with contact melee");
        s.in.trigger=0;s.in.palette=true;Check(!s.Step().meleeContact,"palette disables melee contact");
        s.in.palette=false;s.Step();Check(s.Step().meleeContact,"palette close restores free swing after release");
        s.in.active=false;Check(!s.Step().meleeContact,"menu/tracking inactivity disables contacts");
    }
    {
        Sim s;s.in.palette=true;s.in.hover=2;s.in.trigger=1;s.Step();s.in.palette=false;s.in.trigger=0;s.Step();
        Check(!s.Step().meleeContact,"selected spell suppresses incidental melee");
        ++s.in.equipment;s.Step();Check(s.engine.Selection()==-1,"equipping melee restores attack mode");
        Check(s.Step(3).meleeContact,"new weapon needs no grip release/activation");
    }
    {
        Sim s;s.Nock();s.Move({0,1.3f,0},10);s.in.trigger=0;
        auto shot=s.Step();Check(shot.bowRelease&&!shot.attack&&shot.feedback==2,"draw release shoots");
        Check(shot.rayDirection.z<-.99f&&std::fabs(shot.rayOrigin.z+.5f)<.001,"arrow aims from string through bow hand");
        s.in.hand[0].p.x+=.1f;shot=s.Step();Check(shot.rayDirection.z<-.99f,"release direction stays latched through input pulse");
        Check(!s.Step(5).bowRelease&&!s.out.attack,"release event cannot become a held attack");
        Check(!s.Step(20).bowRelease,"bow cannot repeat without renocking");
    }
    {
        Sim s;s.Nock();Check(s.out.bowDrawing&&s.in.grip[1]==0,"equipped bow needs no left grip clutch");
        s.in.trigger=.4f;Check(s.Step().bowDrawing&&!s.out.attack,"trigger noise cannot release grabbed string");
        s.Move({0,1.3f,0},10);s.in.trigger=.15f;Check(s.Step().bowRelease&&!s.out.attack,"full trigger release emits one shot without pad attack");
    }
    {
        Sim s;s.Nock();s.Move({.2f,1.1f,0},20);
        s.in.hand[1].p={-.05f,1.4f,-.5f};s.Step();s.in.trigger=0;
        const auto expected=math::Normalized(s.in.hand[1].p-s.in.hand[0].p);
        const auto shot=s.Step();
        Check(shot.bowRelease&&!shot.attack&&Length(shot.rayDirection-expected)<.0001f,"release captures both anchors with pitch and yaw");
        s.in.hand[0].p.x+=.08f;s.in.hand[1].p.z+=.08f;
        Check(!s.Step().bowRelease&&Length(s.out.rayDirection-expected)<.0001f,"hand motion after release cannot steer or refire");
    }
    {
        Sim s;s.in.weapon=Weapon::Bow;s.in.hand[1].p={0,1.3f,-.5f};s.in.hand[0].p={.5f,1.3f,-.3f};s.Step(2);
        s.in.trigger=1;Check(!s.Step().bowDrawing,"trigger outside grab volume waits for approach");
        s.Move({0,1.3f,-.34f});Check(s.out.bowDrawing,"pre-squeezed trigger grabs when brought to string");
        s.in.cancel=true;Check(!s.Step().bowRelease&&!s.out.attack,"B cancels draw");s.in.cancel=false;
        Check(!s.Step(3).bowDrawing,"cancelled held trigger cannot renock until released");
    }
    {
        Sim s;s.in.weapon=Weapon::Bow;s.in.hand[1].p={0,1.3f,-.5f};s.in.hand[0].p={0,1.3f,-.12f};s.Step(2);
        s.in.trigger=1;Check(s.Step().bowDrawing,"generous string-side nock region accepts comfortable hand spacing");
        s.Step(10);s.in.trigger=0;Check(!s.Step().bowRelease&&!s.out.attack,"nocking already apart still requires additional deliberate draw");
    }
    {
        Sim s;s.in.palette=true;s.in.hover=3;s.in.trigger=1;s.Step();s.in.palette=false;s.in.trigger=0;s.Step();
        s.in.weapon=Weapon::Bow;++s.in.equipment;s.Step();s.Step();
        Check(s.engine.Selection()==-1&&s.out.bowReady,"equipping a bow automatically enters bow mode");
        s.in.palette=true;s.in.hover=2;s.in.trigger=1;s.Step();s.in.palette=false;s.in.trigger=0;s.Step();
        s.in.active=false;s.Step();s.in.active=true;s.Step(2);
        Check(s.engine.Selection()==2,"tracking or menus preserve an explicitly selected spell on the same bow");
    }
    {
        Sim s;s.Nock();s.in.trigger=0;Check(!s.Step().bowRelease&&!s.out.attack,"short draw does not shoot");
        s.Nock();s.Move({0,1.3f,0});s.in.cancel=true;s.in.trigger=0;Check(!s.Step().bowRelease&&!s.out.attack,"explicit bow cancel never shoots");
    }
    {
        Sim s;s.Nock();s.Move({0,1.3f,0});s.in.tracked[1]=false;Check(!s.Step().bowRelease&&!s.out.attack,"tracking loss cancels draw");
        s.in.tracked[1]=true;s.in.trigger=0;Check(!s.Step(4).bowRelease,"tracking restoration never fires old draw");
        s.Nock();s.Move({0,1.3f,0});++s.in.equipment;s.in.trigger=0;Check(!s.Step().bowRelease&&!s.out.attack,"weapon swap cancels draw");
    }
    {
        Sim s;s.in.palette=true;s.in.hover=5;s.in.trigger=1;
        Check(!s.Step().attack&&s.engine.Selection()==5,"palette selects skill without casting");
        s.in.palette=false;Check(s.Step().skill<0,"palette trigger cannot leak into casting");
        s.in.trigger=0;s.Step();s.in.trigger=1;
        Check(s.Step().skill==5&&!s.out.attack,"selected skill routes separately from default attack");
        Check(SkillButtons(5)==0x4000&&SkillModifier(5),"second-bank slot maps to LT plus X");
        Check(SkillButtons(-1)==0&&!SkillModifier(-1),"invalid skill cannot map to native buttons");
    }
    {
        Sim s;s.in.palette=true;s.in.hover=13;s.in.grip[0]=1;
        Check(s.Step().bottle==1&&s.out.potion<0,"grabbing belt column does not consume it");
        s.in.palette=false;s.Move({0,1.55f,-.10f});
        Check(s.Step(20).potion<0,"upright potion near mouth does not drink");
        s.in.hand[0].q={0,0,.70710678f,.70710678f};
        int drinks=0;
        for(int i=0;i<40;++i) {auto out=s.Step();drinks+=out.potion==1;Check(!out.attack&&out.skill<0,"bottle suppresses combat");}
        Check(drinks==1,"tilted potion dwell consumes once per grab");
        s.in.trigger=1;s.Step();s.in.grip[0]=0;s.Step();
        Check(!s.Step().attack,"bottle release cannot leak held trigger");
    }
    {
        Sim s;s.in.palette=true;s.in.hover=12;s.in.grip[0]=1;s.Step();s.in.active=false;s.Step();s.in.active=true;s.in.palette=false;
        s.Move({0,1.55f,-.1f});s.in.hand[0].q={0,0,1,0};
        for(int i=0;i<30;++i)Check(s.Step().potion<0,"menu cancels carried potion without consuming");
        s.in.trigger=1;s.in.aim.q.w=std::numeric_limits<float>::quiet_NaN();Check(!s.Step().attack,"invalid pose fails closed");
    }
    {
        Sim s;s.in.trigger=1;s.Step();s.in.time+=.2;Check(!s.Step().attack,"long frame gap cancels attacks");
        Check(!s.Step().attack,"stale held trigger must be released");
        for(int row=0;row<4;++row)for(int col=0;col<4;++col)
            Check(PaletteCell((col+.5f)/4,.125f+(row+.5f)*.1875f)==row*4+col,"palette hit regions match atlas");
        Check(PaletteCell(1,.5f)==-1&&PaletteCell(.5f,.95f)==-1,"palette margins never select");
    }
    std::puts("Motion gameplay: gesture, cancellation, input ownership and palette tests passed");
}
