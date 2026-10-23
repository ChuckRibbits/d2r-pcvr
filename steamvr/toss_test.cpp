#include "toss.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace framevr;
static void Check(bool b,const char* message) {if(!b){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main() {
    for(int id:{251,256,257,261,262,271,272,276})Check(toss::Skill(id)&&toss::Model(id),"the eight traps and bombs are thrown, each with a model in the hand");
    for(int id:{273,252,253,258,54,0,-1})Check(!toss::Skill(id)&&!toss::Model(id),"Mind Blast, other skills and none are not");
    Check(toss::Model(261)==toss::Model(271)&&toss::Model(271)==toss::Model(276)&&toss::Model(262)==toss::Model(272)&&
          toss::Model(251)!=toss::Model(256)&&toss::Model(256)!=toss::Model(257)&&toss::Model(261)!=toss::Model(262),
          "the sentries share a model and the wakes another, as the game shares them");
    // Room equal to world but for the anchor; 2 units per metre makes a cell a metre.
    const interaction::Stage stage{true,{0,0,0},{100,0,200},{0,0,0,1}};
    const float fall=std::sqrt(2*1.5f/interaction::ThrowGravity),reach=5*fall;
    float x=0,y=0;
    Check(toss::Landing(stage,0,2,{0,1.5f,0},{5,0,0},x,y)&&std::fabs(x-(100+reach))<1e-3f&&std::fabs(y-200)<1e-3f,
          "a level throw lands as far as its fall time carries it, in grid cells");
    Check(toss::Landing(stage,1,2,{0,2.5f,0},{5,0,0},x,y)&&std::fabs(x-(100+reach-coordinates::GroundShear))<1e-3f&&
          std::fabs(y-(200-coordinates::GroundShear))<1e-3f,"on an upper floor, the renderer's shear comes off");
    float upX=0,upY=0;
    Check(toss::Landing(stage,0,2,{0,1.5f,0},{5,3,0},upX,upY)&&upX>x+reach&&std::fabs(upY-200)<1e-3f,"thrown upward it flies farther");
    const interaction::Stage turned{true,{0,0,0},{100,0,200},math::Yaw(1.5707963f)};
    Check(toss::Landing(turned,0,2,{0,1.5f,0},{5,0,0},x,y)&&std::fabs(x-100)<1e-3f&&std::fabs(std::fabs(y-200)-reach)<1e-3f,
          "the room's turn in the world turns the throw");
    Check(toss::Landing(stage,0,4,{0,1.5f,0},{5,0,0},x,y)&&std::fabs(x-2*(100+reach))<1e-3f,"cells are half a rendered unit");
    Check(!toss::Landing({},0,2,{0,1.5f,0},{5,0,0},x,y),"no room frame, no landing");
    Check(!toss::Landing(stage,0,0,{0,1.5f,0},{5,0,0},x,y)&&!toss::Landing(stage,0,20,{0,1.5f,0},{5,0,0},x,y),"nor without a sane scale");
    Check(!toss::Landing(stage,2,2,{0,1.5f,0},{5,0,0},x,y),"nor from below the floor");
    Check(!toss::Landing(stage,0,2,{0,1.5f,0},{1,40,0},x,y),"nor a flight longer than the potion toss allows");
    Check(!toss::Landing(stage,0,2,{0,1.5f,0},{NAN,0,0},x,y),"nor a bad velocity");
    std::puts("toss tests passed");
}
