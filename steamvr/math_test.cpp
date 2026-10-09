#include "math.h"
#include "input.h"
#include "hero_selection.h"
#include "foveation_math.h"
#include "../cleanroom/camera/mat4.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr::math;
void Near(float actual,float expected) {
    if(!std::isfinite(actual)||std::fabs(actual-expected)>1e-5f) {
        std::fprintf(stderr,"expected %.6f, got %.6f\n",expected,actual);std::exit(1);
    }
}
int main() {
    const float pi=3.14159265358979323846f;
    float p[16];Projection(-.8f,.9f,-.7f,.75f,.05f,p);
    auto ndc=[&](float x,float y,float z,int axis){return (x*p[axis]+y*p[4+axis]+z*p[8+axis]+p[12+axis])/(-z);};
    Near(ndc(std::tan(-.8f),0,-1,0),-1);
    Near(ndc(std::tan(.9f),0,-1,0),1);
    Near(ndc(0,std::tan(-.7f),-1,1),-1);
    Near(ndc(0,std::tan(.75f),-1,1),1);
    Near(ndc(0,0,-.05f,2),1);Near(ndc(0,0,-500,2),.0001f);
    const Vec e{3,2,4};float v[16];View(e,Yaw(pi/2),v);
    for(int axis=0;axis<3;++axis)Near(e.x*v[axis]+e.y*v[4+axis]+e.z*v[8+axis]+v[12+axis],0);
    auto f=Rotate(Yaw(pi/2),{0,0,-1});Near(f.x,-1);Near(f.z,0);
    Near(Heading(Yaw(pi/2)),pi/2);
    // Eye separation remains in metres under a body turn, rather than being
    // multiplied by the hero's dimensions twice.
    auto left=Rotate(Yaw(pi/2),{-.032f,0,0});auto right=Rotate(Yaw(pi/2),{.032f,0,0});
    Near(std::hypot(left.x-right.x,left.z-right.z),.064f);
    // Seated calibration must preserve room movement and both eyes' separation.
    const Vec head{4,1.1f,6};
    const auto origin=ResetHeight({3,1.7f,4},head);
    Near((head-origin).x,1);Near((head-origin).y,0);Near((head-origin).z,2);
    Near((Vec{4,.9f,6}-origin).y,-.2f); // crouching still works after reset
    const auto l=Vec{head.x-.032f,head.y,head.z}-origin;
    const auto r=Vec{head.x+.032f,head.y,head.z}-origin;
    Near(r.x-l.x,.064f);
    framevr::HeightResetButton button;
    auto expect=[](bool condition){if(!condition)std::exit(1);};
    framevr::MenuTriggerGate trigger;
    expect(trigger.Update(true,false)); // normal world attack
    expect(!trigger.Update(true,true)); // menu consumes the same held press
    expect(!trigger.Update(true,false)); // closing menu does not attack
    expect(!trigger.Update(false,false));
    expect(trigger.Update(true,false)); // next deliberate press can attack
    framevr::PressEdge inventory;
    expect(inventory.Update(true));
    expect(!inventory.Update(true)); // holding inventory cannot toggle repeatedly
    expect(!inventory.Update(false));
    expect(inventory.Update(true));
    expect(button.Update(true,.8f)&&!button.MapPressed());
    expect(!button.Update(true,.8f)&&!button.MapPressed());
    expect(!button.Update(true,0)&&!button.MapPressed()); // grip released first
    expect(!button.Update(false,0)&&!button.MapPressed());
    expect(!button.Update(true,0)&&button.MapPressed()); // ordinary click
    expect(!button.Update(true,.8f)&&button.MapPressed()); // grip after click
    button.Update(false,.8f);
    expect(button.Update(true,.8f)&&!button.MapPressed()); // a fresh press
    PanelHit hit;
    expect(IntersectPanel({0,0,0},{0,0,-1},{0,0,-2},Yaw(0),2,1,hit));
    Near(hit.u,.5f);Near(hit.v,.5f);Near(hit.distance,2);
    expect(IntersectPanel({0,0,0},Normalized({.4f,.2f,-2}),{0,0,-2},Yaw(0),2,1,hit));
    Near(hit.u,.7f);Near(hit.v,.3f);
    expect(!IntersectPanel({0,0,0},{0,0,-1},{0,0,2},Yaw(0),2,1,hit));
    expect(!IntersectPanel({0,0,0},Normalized({3,0,-2}),{0,0,-2},Yaw(0),2,1,hit));
    const auto rotated=Yaw(pi/2);
    expect(IntersectPanel({0,0,0},Rotate(rotated,{0,0,-1}),Rotate(rotated,{0,0,-2}),rotated,2,1,hit));
    Near(hit.u,.5f);Near(hit.v,.5f);
    const Vec beamDir=Normalized({.2f,.3f,-1}),towardHead={0,1,.2f};
    const auto beam=BeamOrientation(beamDir,towardHead);
    const auto beamUp=Rotate(beam,{0,1,0}),beamNormal=Rotate(beam,{0,0,1});
    Near(Dot(beamUp,beamDir),1);Near(Dot(beamNormal,beamDir),0);
    expect(Dot(beamNormal,towardHead)>0);
    expect(framevr::LocalStandingBody(100,200,100,200,6,6));
    expect(framevr::LocalStandingBody(101,201,100,200,3,6)); // crouching, sub-tile camera offset
    expect(!framevr::LocalStandingBody(100,200,100,200,.5f,6)); // corpse at respawn
    expect(!framevr::LocalStandingBody(110,200,100,200,6,6)); // another player rig
    expect(!framevr::LocalStandingBody(100,200,100,200,6,NAN));
    // Fixed foveation preserves the optical center, whole boundary tiles,
    // asymmetric eye centers, and the unscaled area outside a scene viewport.
    expect(framevr::FixedRate(31,31,16,0,0,1024,1024,.5f,.5f,.55f)==0);
    expect(framevr::FixedRate(0,0,16,0,0,1024,1024,.5f,.5f,.55f)==5);
    expect(framevr::FixedRate(49,31,16,0,0,1024,1024,.5f,.5f,.55f)==0);
    expect(framevr::FixedRate(50,31,16,0,0,1024,1024,.5f,.5f,.55f)==5);
    expect(framevr::FixedRate(11,19,16,16,0,800,640,.2f,.5f,.55f)==0);
    expect(framevr::FixedRate(60,60,16,0,0,800,640,.5f,.5f,.55f)==0);
    // Both eye label projections and their inverse rays must meet the same
    // creature anchor, including asymmetric FOV and a rotated world view.
    const auto attitude=Multiply(Yaw(.5f),Multiply({std::sin(.15f),0,0,std::cos(.15f)},{0,0,std::sin(.2f),std::cos(.2f)}));
    const Vec headOrigin{5,2,-7},anchor=headOrigin+Rotate(attitude,{1.2f,.8f,-6.f});
    for(int e=0;e<2;++e){
        const Vec pos=headOrigin+Rotate(attitude,{e? .032f:-.032f,0,0});float view[16],proj[16],pixel[2];
        View(pos,attitude,view);Projection(e?-.7f:-.9f,e?.9f:.7f,-.8f,.8f,.05f,proj);
        const float xyz[]={anchor.x,anchor.y,anchor.z};
        expect(d2rcam::m4::ProjectToPixels(xyz,view,proj,1920,1080,pixel));
        const float x=2*pixel[0]/1920-1,y=1-2*pixel[1]/1080;
        const auto recovered=pos+Rotate(attitude,{((x+proj[8])/proj[0])*6,((y+proj[9])/proj[5])*6,-6});
        Near(recovered.x,anchor.x);Near(recovered.y,anchor.y);Near(recovered.z,anchor.z);
    }
    float labelView[16],labelProj[16],pixel[2];View({0,0,0},Yaw(0),labelView);Projection(-.8f,.8f,-.8f,.8f,.05f,labelProj);
    const float behind[]={0,0,1},tooNear[]={0,0,-.01f},badPoint[]={NAN,0,-1};
    expect(!d2rcam::m4::ProjectToPixels(behind,labelView,labelProj,1920,1080,pixel));
    expect(!d2rcam::m4::ProjectToPixels(tooNear,labelView,labelProj,1920,1080,pixel));
    expect(!d2rcam::m4::ProjectToPixels(badPoint,labelView,labelProj,1920,1080,pixel));
    std::puts("OpenXR camera math passed");
}
