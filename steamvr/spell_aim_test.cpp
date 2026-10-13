#include "spell_aim.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace framevr;
static void Check(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main() {
    Check(spellaim::Directed(0,-1,8),"Teeth fan is directional without a primary missile field");
    Check(spellaim::Directed(0,36,0)&&spellaim::Directed(0,395,0),"primary missile behavior is independent of class/skill id");
    Check(spellaim::Directed(0,-1,19)&&spellaim::Directed(0,-1,10),"streams and initially aimed homing casts retain hand direction");
    for(int function:{22,28,56,60,63,68,123})Check(!spellaim::Directed(0,-1,function),"nova, ground, summon, corpse and buff behaviors retain native targeting");
    for(uint32_t flag:{1u<<4,1u<<20,1u<<24})Check(!spellaim::Directed(flag,12,8),"passive and unit/corpse-only skills never lose required targets");
    Check(!spellaim::Directed(0,-1,0)&&!spellaim::Directed(0,-1,999),"unknown behavior is not guessed to be projectile");
    spellaim::Cast context{};
    {
        spellaim::Scope cast(context,{true,.6f,.8f});
        {
            spellaim::Scope nested(context,{});
            Check(!context.directed,"nested non-projectile resolution cannot inherit target suppression");
        }
        Check(context.directed&&context.x==.6f,"nested return restores sampled direction for remaining fan resolution");
    }
    Check(!context.directed,"native targeting restored after the cast");
    for(int degree=-180;degree<=180;++degree) {
        const float angle=degree*3.14159265f/180;
        for(float pitch:{-.9f,0.f,.9f}) {
            const math::Vec direction{std::cos(angle)*std::cos(pitch),std::sin(pitch),std::sin(angle)*std::cos(pitch)};
            float x=0,y=0;
            Check(spellaim::Point(1200,1300,direction,x,y),"finite directional spell yields native aim point");
            const auto rounded=math::Normalized(math::Vec{float(int(x)-1200),0,float(int(y)-1300)});
            Check(rounded.x*std::cos(angle)+rounded.z*std::sin(angle)>.9995f,"all horizontal aim angles remain accurate after native cell quantization");
        }
    }
    float x=5,y=7;
    for(int degree=-180;degree<=180;degree+=5) {
        const float angle=degree*3.14159265f/180;
        const spellaim::Cast cast{true,std::cos(angle),std::sin(angle)};
        // Native resolution can return the movement target without invoking
        // AttackPoint. Simulate forward, backward, strafing and idle results.
        for(float travel:{0.f,1.57079633f,3.14159265f,-1.57079633f})for(float distance:{0.f,8.f}) {
            const float playerX=1200.25f+std::cos(angle+travel)*distance;
            const float playerY=1300.75f+std::sin(angle+travel)*distance;
            x=playerX+std::cos(angle+travel)*3;y=playerY+std::sin(angle+travel)*3;
            Check(spellaim::Resolve(cast,playerX,playerY,x,y),"moving cast overrides every native resolver result");
            const auto actual=math::Normalized(math::Vec{x-playerX,0,y-playerY});
            Check(actual.x*cast.x+actual.z*cast.y>.99999f,"projectile aim is independent of forward, backward or sideways travel");
            int cx=int(playerX-3*cast.x),cy=int(playerY-3*cast.y);
            Check(spellaim::ControllerPoint(playerX,playerY,{cast.x,0,cast.y},cx,cy),"controller quick cast also replaces backward walking destination");
            const auto controller=math::Normalized(math::Vec{cx-playerX,0,cy-playerY});
            Check(controller.x*cast.x+controller.z*cast.y>.9995f,"controller quick-cast integer target retains hand direction");
            Check(std::fabs(std::hypot(x-playerX,y-playerY)-60)<.001f,"cast target follows current player position at full aiming precision");
        }
    }
    x=5;y=7;
    Check(!spellaim::Resolve({},1200,1300,x,y)&&x==5&&y==7,"non-projectile native targets are preserved");
    Check(spellaim::Resolve({true,23,42,true},1200,1300,x,y)&&x==23&&y==42,"shield contact keeps its exact point instead of projectile range");
    x=5;y=7;
    Check(!spellaim::Resolve({true,NAN,0},1200,1300,x,y)&&x==5&&y==7,"invalid cast aim preserves native result");
    Check(!spellaim::Resolve({true,1,0},NAN,1300,x,y)&&x==5&&y==7,"invalid player position preserves native result");
    Check(!spellaim::Point(0,0,{0,1,0},x,y)&&x==5&&y==7,"vertical aim cannot invent a horizontal direction");
    Check(!spellaim::Point(0,0,{NAN,0,1},x,y),"invalid direction is refused");
    int cx=12,cy=34;
    Check(!spellaim::ControllerPoint(NAN,0,{1,0,0},cx,cy)&&cx==12&&cy==34,"invalid controller position cannot change output");
    Check(!spellaim::ControllerPoint(65530,100,{1,0,0},cx,cy),"out-of-world controller target is refused before integer conversion");
    std::puts("Spell aiming: native behavior, scoped targeting, movement-independent cast resolution and cell precision passed");
}
