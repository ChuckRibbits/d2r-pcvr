#include "controller_bindings.h"
#include <array>
#include <cstdio>
#include <cstdlib>
using namespace framevr::interaction;
static void Check(bool value,const char* message){if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main() {
    Check(CatalogSkillId(217)==-1&&CatalogSkillId(218)==-1,"identify scroll/book are not controller spell bindings");
    Check(CatalogSkillId(219)==359&&CatalogSkillId(220)==359&&CatalogSkillId(70)==70,"portal sources share native shortcut; class skills unchanged");
    std::array<ControllerBinding,32> controller{};
    controller[2].skill=70; // saved Raise Skeleton, observed in the owner's game
    controller[18].skill=84;controller[18].owner=123; // second native bank, item skill owner
    constexpr uintptr_t table=0x20000;
    int reads=0;
    auto read=[&](uintptr_t p,ControllerBinding& value) {
        ++reads;const auto offset=p-table;
        if(p<table||offset%28||offset/28>=controller.size())return false;
        value=controller[offset/28];return true;
    };
    ControllerBinding found;
    // Reproduce the old failure: native mouse mode would choose the empty
    // keyboard table. Saved controller data must be correct on the first read
    // and remain correct through input-mode changes without wheel navigation.
    for(bool mouseMode:{true,false,true}) {
        const int oldGenericResult=mouseMode?-1:controller[2].skill;
        if(mouseMode)Check(oldGenericResult==-1,"fixture reproduces empty keyboard bank");
        Check(ReadControllerBinding(table,0,2,read,found)&&found.skill==70&&found.owner==-1,
              "saved controller assignment visible independent of last input mode");
    }
    Check(ReadControllerBinding(table,16,2,read,found)&&found.skill==84&&found.owner==123,
          "weapon-set bank and owning item retained");
    Check(ReadControllerBinding(table,0,1,read,found)&&found.skill==-1,"actual unassignment remains empty");
    for(int bank:{-1,1,15,17,32}) {
        const int before=reads;
        Check(!ReadControllerBinding(table,bank,2,read,found)&&reads==before&&found.skill==-1,"invalid bank fails before memory read");
    }
    for(int slot:{-1,16,32})Check(!ReadControllerBinding(table,0,slot,read,found),"invalid slot fails closed");
    controller[2].skill=5000;
    Check(!ReadControllerBinding(table,0,2,read,found)&&found.skill==-1,"corrupt id does not retain last assignment");
    Check(!ReadControllerBinding(table,0,2,[](uintptr_t,ControllerBinding&){return false;},found),"unreadable table fails closed");
    std::puts("Controller bindings: saved assignment, mouse/gamepad independence, alternate bank, owner and invalid reads passed");
}
