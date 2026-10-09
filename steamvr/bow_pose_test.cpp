#include "../vr/bow_pose.h"
#include "gameplay.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace skel::bow;
static void Check(bool ok,const char* label) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",label);std::exit(1);}}
static bool Near(Vec a,Vec b) {return Length(a-b)<.0001f;}
static Pose SyntheticBow() {
    Pose p;for(auto& m:p)m=Identity();
    Position(p[4],{0,.3f,0});Position(p[5],{0,-.3f,0});
    Position(p[6],{0,-2,-.5f});Position(p[7],{0,2,-.5f});
    Position(p[8],{0,0,-.5f});Position(p[9],{0,-1,-.5f});Position(p[10],{0,1,-.5f});
    Position(p[11],{0,0,-.48f});Position(p[12],{0,0,-.48f});return p;
}
int main() {
    const Pose bind=SyntheticBow();Rig rig;Check(rig.Learn(bind),"synthetic bow accepted");
    Pose posed;
    for(int step=0;step<=100;++step) {
        const float draw=step/100.f;Check(rig.Draw(Identity(),draw,true,posed),"full draw range solves");
        for(int side=0;side<2;++side) {
            const int hinge=side?5:4,tip=side?6:7;
            Check(Near(Position(posed[hinge]),Position(bind[hinge])),"grip and limb hinges stay fixed");
            Check(std::fabs(Length(Position(posed[tip])-Position(posed[hinge]))-Length(Position(bind[tip])-Position(bind[hinge])))<.0001f,"limb length preserved");
            Check(std::fabs(Length(Position(posed[8])-Position(posed[tip]))-Length(Position(bind[8])-Position(bind[tip])))<.0001f,"string length preserved");
        }
        Check(Near(Position(posed[11])-Position(posed[8]),Position(bind[11])-Position(bind[8])),"arrow remains nocked during pull");
    }
    Mat placement=Rotation({0,0,1},{1,0,0});Position(placement,{3,7,1});
    Check(rig.Draw(placement,.6f,true,posed),"rotated translated bow solves");Pose local;rig.Draw(Identity(),.6f,true,local);
    for(int i=0;i<Count;++i)Check(Near(Position(posed[i]),Vector(placement,Position(local[i]))+Position(placement)),"pose follows bow placement");
    Check(rig.Draw(Identity(),0,false,posed),"released bow solves");
    Check(Near(Position(posed[8]),Position(bind[8])),"release immediately returns string, no delayed windup");
    Check(posed[11].m[0]<.002f&&posed[12].m[0]<.002f,"released arrow and helper hidden");
    {
        // A translated, scaled, mirrored attachment like a left wrist. Solve
        // a 3D two-hand target in item space, then check it back in head space.
        Mat attach=Rotation({0,0,1},{1,0,0});
        for(int i=0;i<3;++i)for(int j=0;j<3;++j)attach.m[4*i+j]*=(i==0?-.25f:.25f);
        Position(attach,{-.2f,-.3f,-.5f});Mat undo;Check(Inverse(attach,undo),"left-hand attachment invertible");
        const Vec localNock={.3f,.2f,-1.7f},localLine={-.3f,-.2f,1.7f};
        const Vec rightHand=Vector(attach,localNock)+Position(attach),leftHand=rightHand+Vector(attach,localLine);
        const Vec nock=Vector(undo,rightHand)+Position(undo),line=Vector(undo,leftHand-rightHand);
        Check(rig.DrawAt(Identity(),nock,line,true,posed),"lateral and pitched draw solves");
        Check(Near(Vector(attach,Position(posed[String]))+Position(attach),rightHand),"string exactly follows drawing hand in 3D");
        const Vec arrowDirection=Normalized(Vector(attach,Vector(posed[Arrow],{0,0,1})));
        Check(Dot(arrowDirection,Normalized(leftHand-rightHand))>.9999f,"visible arrow and two-hand shot vector agree after mirror/rotation/scale");
        Check(rig.DrawAt(Identity(),{30,20,-50},{0,0,1},true,posed),"unreachable draw constrained safely");
    }
    Pose invalid=bind;invalid[4].m[0]=0;Check(!Rig{}.Learn(invalid),"singular bind rejected");
    invalid=bind;invalid[8].m[14]=0;Check(!Rig{}.Learn(invalid),"missing pull axis rejected");
    const Pose saved=posed;
    Check(!rig.Draw(Identity(),std::numeric_limits<float>::quiet_NaN(),true,posed),"NaN draw rejected");
    for(int i=0;i<Count;++i)for(int j=0;j<16;++j)Check(saved[i].m[j]==posed[i].m[j],"failed pose leaves output unchanged");
    // Integrate the actual gesture recognizer: visible bend before any native
    // attack, held pose sustained, one release pulse, cancellation never fires.
    using framevr::gameplay::Input;using framevr::gameplay::Engine;using framevr::gameplay::Weapon;
    Engine engine;Input in;in.active=in.tracked[0]=in.tracked[1]=in.aimTracked=true;in.weapon=Weapon::Bow;
    in.head.p={0,1.7f,0};in.hand[0].p={0,1.3f,-.4f};in.hand[1].p={0,1.3f,-.5f};
    auto step=[&](){in.time+=.02;return engine.Step(in);};step();step();in.trigger=1;
    Check(step().bowDrawing,"nock starts live pose");
    for(int i=0;i<20;++i){in.hand[0].p.z+=.02f;auto o=step();Check(o.bowDrawing&&!o.attack,"pull does not request native attack");Check(rig.Draw(Identity(),o.draw,o.bowDrawing,posed),"gesture drives native pose");}
    Check(Position(posed[8]).z<Position(bind[8]).z-1,"string bends while trigger held");
    auto held=step();Check(held.bowDrawing&&!held.attack,"holding draw stays held");
    in.trigger=0;auto released=step();Check(released.attack&&!released.bowDrawing,"only release requests native shot");
    rig.Draw(Identity(),released.bowDrawing?released.draw:0,released.bowDrawing,posed);
    Check(Near(Position(posed[8]),Position(bind[8])),"release resets live pose on same sample");
    for(int i=0;i<8;++i) {in.aim.q=framevr::math::Yaw(float(i));auto o=step();Check(Near(o.rayDirection,released.rayDirection),"release aim remains latched through native windup");}
    in.cancel=true;Check(!step().attack,"cancel clears release pulse");
    std::puts("Bow pose: geometry, draw/release timeline, placement, visibility and invalid-input tests passed");
}
