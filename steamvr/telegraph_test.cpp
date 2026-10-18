#include "telegraph.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr::telegraph;
static void Check(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
static int Showing(const Marks& marks,uint64_t now){int n=0;for(const auto& m:marks)n+=Sample(m,now).on;return n;}
int main() {
    Check(ExpectedFlight(0,0,28,0)==2000,"flight time follows the measured bomb speed");
    Check(ExpectedFlight(5,5,5,5)==MinFlightMs,"a throw at your feet still shows briefly");
    Check(!Sample({},1000).on,"nothing thrown, nothing drawn");
    const Mark m{100,200,1000,2000,0,1};
    Check(!Sample(m,999).on,"not before the throw");
    auto d=Sample(m,1000);Check(d.on&&d.fill==0&&d.alpha==1,"appears the moment the bomb is thrown, unfilled");
    d=Sample(m,2000);Check(d.on&&d.fill>.49f&&d.fill<.51f,"half filled halfway through the flight");
    d=Sample(m,3000);Check(d.on&&d.fill==1&&d.alpha==1,"full when the bomb should land");
    d=Sample(m,3000+LateMs+FadeMs/2);Check(d.on&&!d.burst&&d.alpha>.4f&&d.alpha<.6f,"a missed burst event still fades out");
    Check(!Sample(m,3000+LateMs+FadeMs).on,"and then disappears");
    Mark landed=m;landed.landed=1800;
    d=Sample(landed,1800);Check(d.on&&d.burst&&d.fill==1&&d.alpha==1,"burst completes the fill and flashes");
    Check(!Sample(landed,1800+FadeMs).on,"gone after the burst fade");

    // Several bombs in the air keep their own circles.
    Board board;
    board.Cast(0,0,28,0,1000);board.Cast(0,0,0,28,1200);board.Cast(0,0,-28,0,1400);
    Check(Showing(board.Get(),1500)==3,"three throws show three circles");
    auto ended=board.Landed(1,27,2900);
    Check(ended.cast==1200&&ended.y==28,"a burst ends the circle it landed in, not the newest");
    Check(Showing(board.Get(),2900+FadeMs)==2,"the other two circles stay");
    ended=board.Landed(500,500,3100);
    Check(ended.cast==1000,"a burst far from every circle ends the oldest airborne one");
    Check(board.Landed(-28,0,3300).cast==1400&&!board.Landed(-28,0,3400).cast,"each circle ends once");
    for(int i=0;i<MaxMarks+2;++i)board.Cast(0,0,float(i),0,10000+i);
    const auto full=board.Get();
    Check(Showing(full,10010)==MaxMarks,"more throws than slots reuse the oldest circles");
    bool newest=false;for(const auto& k:full)newest|=k.x==float(MaxMarks+1);
    Check(newest,"the newest throw always gets a circle");
    // Placed skills (traps, Blizzard): fill over their time, then fade; repeats merge.
    Board placed;
    Check(placed.Place(50,50,20000,1000,1,4.f),"a Blizzard cast places a circle");
    Check(!placed.Place(50.5f,50,20100,1000,1,4.f),"the same cast repeating at once is not a second circle");
    auto marks=placed.Get();int found=-1;for(int i=0;i<MaxMarks;++i)if(marks[i].cast==20000)found=i;
    Check(found>=0&&marks[found].element==1&&marks[found].radius==4.f,"the circle keeps its element and radius");
    d=Sample(marks[found],20500);Check(d.on&&d.fill>.49f&&d.fill<.51f,"half filled halfway");
    Check(!Sample(marks[found],21000+FadeMs).on,"gone after its time and the fade");
    Check(placed.Place(60,50,20100,600,2,2.5f),"another point at once is its own circle");
    // Measured areas: Blizzard wide and long, Meteor filling until it lands, Fire Wall a strip.
    Check(ForSkill(59).radius==7.f&&ForSkill(59).showMs==4100&&ForSkill(59).halfLength==0,"Blizzard: 7 cells for the 4.1 s storm");
    Check(ForSkill(56).showMs==2650&&ForSkill(56).radius==5.f,"Meteor: 5 cells, full when it lands at 2.65 s");
    Check(ForSkill(51).halfLength==5.f&&ForSkill(51).radius<1.f,"Fire Wall: a narrow 10-cell strip");
    Board wall;
    Check(wall.Place(10,10,30000,700,0,.75f,0,4,5),"a strip places");
    marks=wall.Get();found=-1;for(int i=0;i<MaxMarks;++i)if(marks[i].cast==30000)found=i;
    Check(found>=0&&marks[found].dirX==0&&marks[found].dirY==1&&marks[found].halfLength==5.f,"its direction is kept as a unit vector");
    Check(wall.Place(40,10,30000,700,0,2,0,0,5)&&[&]{for(const auto& k:wall.Get())if(k.x==40)return k.halfLength==0;return false;}(),"no direction: a circle");
    std::puts("Landing circle: timing, several bombs, burst matching and reuse passed");
}
