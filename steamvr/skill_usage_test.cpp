#include "skill_usage.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
using namespace framevr;
static int failures=0;
static void Check(bool ok,const char* what){if(!ok){std::printf("FAIL: %s\n",what);++failures;}}
int main() {
    using namespace skillusage;
    const int rightTrigger=skills::TriggerSlot(0),leftTrigger=skills::TriggerSlot(1),rightFace=skills::HandSlot(0,1),leftFace=skills::HandSlot(1,2);
    for(int skill=0;skill<400;++skill)for(int slot=0;slot<skills::Count;++slot) {
        const char* text=Describe(skill,slot);
        if(!text[0]||std::strlen(text)>MaxLength){Check(false,"every skill and slot has a line that fits");break;}
    }
    Check(!Describe(-1,rightTrigger)[0]&&!Describe(126,-1)[0]&&!Describe(126,skills::Count)[0],"no skill or no slot: nothing");
    Check(Gesture(strikeskills::Bash,rightTrigger)&&Gesture(strikeskills::Bash,leftTrigger)&&!Gesture(strikeskills::Bash,rightFace),
          "a strike skill is a gesture on either trigger, a press on a face button");
    Check(std::strstr(Describe(strikeskills::Zeal,rightTrigger),"next hit")&&std::strstr(Describe(strikeskills::Fury,rightTrigger),"next hit"),
          "Zeal and Fury say their later hits come from later swings");
    Check(std::strstr(Describe(strikeskills::Bash,rightFace),"On a trigger it is a gesture"),"a face button points to the trigger gesture");
    Check(Gesture(weaponskills::Smite,leftTrigger)&&!Gesture(weaponskills::Smite,rightTrigger)&&std::strstr(Describe(weaponskills::Smite,leftTrigger),"shield"),
          "Smite: a shield gesture on the left trigger");
    Check(Gesture(weaponskills::Strafe,rightTrigger)&&Gesture(weaponskills::Strafe,leftTrigger)&&!Gesture(weaponskills::Strafe,leftFace),"arrow skills: draw and release on a trigger");
    Check(std::strstr(Describe(7,rightFace),"loads it into every following arrow")!=nullptr,"a quiver arrow skill on a face button loads the quiver");
    Check(Gesture(15,rightTrigger)&&!Gesture(15,leftTrigger)&&std::strstr(Describe(15,leftFace),"following throw"),"javelin skills: the right trigger's throw, or loaded");
    Check(Gesture(weaponskills::DoubleThrow,rightTrigger)&&!Gesture(weaponskills::DoubleThrow,leftTrigger),"Double Throw on the right trigger");
    Check(Gesture(251,rightTrigger)&&!Gesture(251,leftTrigger)&&!Gesture(251,rightFace),"traps and bombs are thrown from the right trigger only");
    Check(!Gesture(36,rightTrigger)&&std::strstr(Describe(36,rightTrigger),"trigger")&&std::strstr(Describe(36,rightFace),"button"),
          "any other skill casts natively from its trigger or button");
    Check(std::strstr(Describe(Interact,skills::HandSlot(0,1)),"pick up")&&std::strstr(Describe(weaponskills::Attack,rightTrigger),"attack"),
          "Interact and the basic attack say what they do, not \"cast\"");
    if(failures)return 1;
    std::puts("skill usage lines: every skill and slot fits; gestures named by slot");
    return 0;
}
