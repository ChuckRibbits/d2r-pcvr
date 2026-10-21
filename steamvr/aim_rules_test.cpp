#include "aim_rules.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr::aimrules;
static void Check(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main() {
    Check(Of(36,true)==Use::Heading&&Of(12,true)==Use::Heading,"bolts and fans follow the target's heading");
    Check(Of(59,false)==Use::Ground&&Of(251,true)==Use::Ground&&Of(78,false)==Use::Ground&&Of(88,false)==Use::Ground,"Blizzard, Fire Blast and the bone walls are ground skills");
    Check(Of(66,false)==Use::Curse&&Of(87,false)==Use::Curse&&Of(91,false)==Use::Curse,"Necromancer curses centre on the target");
    Check(Of(107,false)==Use::Attack&&Of(143,false)==Use::Attack&&Of(275,false)==Use::Attack&&Of(151,false)==Use::Attack,"Charge, Leap Attack, Dragon Flight, Whirlwind attack the target");
    Check(Of(54,false)==Use::Spot&&Of(132,false)==Use::Spot&&Of(70,false)==Use::Spot,"Teleport, Leap and Raise Skeleton go to the pointed spot");
    Check(Of(74,false)==Use::CorpseAtTarget,"Corpse Explosion goes to the target");
    Check(Of(26,true)==Use::Native&&Of(149,false)==Use::Native,"Strafe and self skills are the game's");
    Check(Placed(Use::Curse)&&Placed(Use::Spot)&&!Placed(Use::Heading)&&!Placed(Use::Native),"curses and spots are placed, bolts are not");
    Check(TakesTarget(Use::Attack)&&!TakesTarget(Use::Spot)&&!TakesTarget(Use::Native),"travel spots never take the target");
    Check(Circle(Use::Ground)&&!Circle(Use::Curse),"curses are instant: no landing circle");
    Check(FrontOfTarget(78)==3&&FrontOfTarget(88)==0&&FrontOfTarget(59)==0,"Bone Wall in front of the target, Bone Prison on it");
    Check(WantsUnit(22)&&WantsUnit(121)&&WantsUnit(107)&&WantsUnit(106)&&!WantsUnit(36)&&!WantsUnit(54)&&!WantsUnit(66),"seekers, Fist of the Heavens, Charge and Zeal get the monster; bolts, Teleport and curses do not");
    const auto right=For(0,false),left=For(1,false),bow=For(0,true);
    Check(right.at[0]==Right&&right.at[1]==Left&&right.at[2]==Eyes,"a right button: right, left, eyes");
    Check(left.at[0]==Left&&left.at[1]==Right&&left.at[2]==Eyes,"a left button: left, right, eyes");
    Check(bow.at[0]==Left&&bow.at[1]==Right,"a bow skill on a right button: the bow hand first");
    std::puts("aim rules ok");
    return 0;
}
