#include "aim_trace.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr::aimtrace;
static void Check(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main() {
    uint64_t last=0;
    Check(Due(last,5000,MissGapMs)&&!Due(last,5999,MissGapMs)&&Due(last,6000,MissGapMs),"a hand-off miss is logged at most once a second");

    Window w;w.Game(1);
    Check(!w.Arm(84,1000,0,0)&&!w.Arm(93,1000,0,0),"only Bone Wall and Bone Prison open the wall window");
    Check(w.Arm(78,1000,10,20)&&w.Open(1000)&&w.Open(1000+WindowMs-1)&&!w.Open(1000+WindowMs),"a Bone Wall cast opens a 6 s window");
    Check(!w.Arm(88,2000,0,0)&&w.skill==78&&w.x==10&&w.y==20,"a cast inside the window does not restart it");
    Check(w.SampleDue(1000)&&!w.SampleDue(1099)&&w.SampleDue(1100),"samples at most every 100 ms");
    Check(!w.SampleDue(1000+WindowMs),"no sample after the window");
    Check(!w.Closed(2000)&&w.Closed(1000+WindowMs)&&!w.Closed(1000+WindowMs+1),"the close is reported once");
    Check(w.Arm(88,10000,0,0)&&w.Closed(20000)&&w.Arm(78,30000,0,0)&&w.used==3,"three casts traced");
    w.Closed(40000);
    Check(!w.Arm(78,40000,0,0),"the fourth cast of a game is not traced");
    w.Game(1);
    Check(!w.Arm(78,50000,0,0),"the same game keeps the budget");
    w.Game(2);
    Check(w.used==0&&w.Arm(88,60000,0,0),"a new game resets the budget");
    Check(Within(3,4,0,0,5)&&!Within(3,4.1f,0,0,5),"within is a radius in cells");

    Tracker<4> t;
    t.Base(1,7);
    t.Begin();
    Check(!t.Seen(1,7,100)&&t.Seen(1,8,100)&&t.Seen(3,8,100)&&!t.Seen(1,8,200),"the baseline is not new; a type and id is new once");
    int gone=0;uint32_t goneId=0;
    t.Seen(3,8,150,4,5);
    t.Begin();t.Seen(1,8,300);
    Check(t.Gone([&](const Unit& u){++gone;goneId=u.id;Check(u.type==3&&u.at==100&&u.x==4&&u.y==5,"gone keeps its type, first sighting and last place");})==1&&gone==1&&goneId==8,
          "a new unit not seen in a sample is gone");
    t.Begin();
    Check(t.Gone([](const Unit&){})==1&&t.count==1,"the baseline is never reported gone");
    Check(t.Seen(1,9,400)&&t.Seen(1,10,400)&&t.Seen(1,11,400)&&!t.Seen(1,12,400)&&t.count==4,"a full tracker drops the rest");
    t.Clear();
    Check(t.count==0&&t.Seen(1,7,500),"cleared for the next cast");
    std::puts("aim trace: ok");
}
