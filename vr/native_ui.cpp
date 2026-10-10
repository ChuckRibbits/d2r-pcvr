#include <windows.h>
#include <D2RLPlugin/api.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <map>
#include <string>
#include "native_ui.h"
#include "native_ui_signatures.h"
#include "sigscan.h"
#include "memory.h"
#include "combat.h"
#include "logging.h"
namespace nativeui {
namespace {
const D2RL::PluginContext* context=nullptr;
const D2RL::LocalizationService* strings=nullptr;
std::map<int,std::string> names;
bool ready=false;
uint64_t retryAt=0;
template<class T>bool Read(uintptr_t p,T& t){return p>0x10000&&framevr::ReadMemory(&t,reinterpret_cast<void*>(p),sizeof t);}
bool Ready() {
    if(ready)return true;
    if(GetTickCount64()<retryAt)return false;
    retryAt=GetTickCount64()+1000;
    using namespace abi;
#define CHECK(x) if(!d2rsig::Check(x##Rva,x##Sig,sizeof x##Sig))return false
    CHECK(LocalIndex);CHECK(ClientPlayer);CHECK(ButtonSlot);CHECK(SlotLayout);CHECK(Binding);CHECK(BindingLayout);
    CHECK(Owner);CHECK(Skill);CHECK(SkillLayout);CHECK(SkillList);CHECK(Stat);CHECK(StatBody);CHECK(StatLookup);
#undef CHECK
    ready=true;framevr::logging::Write("Frame interactions: native controller assignments and resource readers verified");return true;
}
const char* SkillKey(int id,char key[80]) {
    // Original localization keys are not fully regular across the expansion.
    // These are identifiers, not copied text/images from the game.
    if(id==61)return "d2r:skillsname61";
    if(id==197)return "d2r:Skillname226";
    if(id==350)return "d2r:Runeword22";
    if(id>=357&&id<=364){const char* special[]={"SkillNameInteract","SkillNameLoot","skillname219","SkillNameEmoteWheel","SkillNameSwapWeapons","SkillNameMap","SkillNameShowItems","SkillNameRunToggle"};std::snprintf(key,80,"d2r:%s",special[id-357]);return key;}
    if(id==370)return "d2r:SkillNameCubeLoot";
    if(id==371)return "d2r:SkillnameMetamorphosisBear";
    if(id==372)return "d2r:SkillnameMetamorphosisWolf";
    if(id>372)return nullptr; // table/localization adapter needed; no fabricated name
    std::snprintf(key,80,"d2r:skillname%d",id+(id>=221&&id<=280?1:0));return key;
}
std::string Name(int id) {
    if(auto i=names.find(id);i!=names.end())return i->second;
    char key[80]{},text[256]{};const char* k=SkillKey(id,key);
    if(k&&strings&&strings->getStringByKey(context,k,text,sizeof text,nullptr)==D2RL::Localization::Result::Success)
        return names.emplace(id,text).first->second;
    std::snprintf(text,sizeof text,"Skill %d",id);return text;
}
}
void Init(const D2RL::PluginContext* c){context=c;c->QueryService(&strings);}
void Capture(framevr::interaction::Snapshot& s) {
    if(!s.session||!Ready())return;
    const int index=reinterpret_cast<int(*)()>(d2rsig::Addr(abi::LocalIndexRva))();
    if(index<0||index>=8)return;
    const auto player=reinterpret_cast<uintptr_t(*)(int)>(d2rsig::Addr(abi::ClientPlayerRva))(index);
    uint32_t h[3]{},id=0;
    if(!Read(player,h)||h[0]!=0||!combat::LocalPlayerId(id)||id!=h[2])return;
    auto stat=reinterpret_cast<int(*)(uintptr_t,int,int)>(d2rsig::Addr(abi::StatRva));
    s.health=stat(player,6,0)/256.f;s.maxHealth=stat(player,7,0)/256.f;
    s.mana=stat(player,8,0)/256.f;s.maxMana=stat(player,9,0)/256.f;
    s.vitalsKnown=s.health>=0&&s.maxHealth>0&&s.health<=s.maxHealth*2&&s.maxHealth<1000000&&
        s.mana>=0&&s.maxMana>=0&&s.mana<=std::max(1.f,s.maxMana)*2&&s.maxMana<1000000;
    // Lookup asserts on a missing skill list during load/teardown. The
    // signature-checked accessor's +0x100 layout is guarded separately.
    uintptr_t skillList=0;
    if(!Read(player+0x100,skillList)||skillList<0x10000)return;
    const int buttons[]={0x2000,0x4000,0x8000,0x200};
    for(int i=0;i<8;++i) {
        const int slot=reinterpret_cast<int(*)(int,bool)>(d2rsig::Addr(abi::ButtonSlotRva))(buttons[i%4],i>=4);
        if(slot<0||slot>=16)continue;
        auto& skill=s.skills[i];
        const int skillId=reinterpret_cast<int(*)(int)>(d2rsig::Addr(abi::BindingRva))(slot);
        if(skillId<-1||skillId>4095)continue;
        skill.id=skillId;skill.known=true;
        if(skillId==-1){std::snprintf(skill.name,sizeof skill.name,"Unassigned");continue;}
        const int owner=reinterpret_cast<int(*)(int)>(d2rsig::Addr(abi::OwnerRva))(slot);
        const auto node=reinterpret_cast<uintptr_t(*)(uintptr_t,int,int)>(d2rsig::Addr(abi::SkillRva))(player,skillId,owner);
        // Presence is availability of the assignment, not a mana/cooldown
        // guarantee. Those gates remain in the native casting path.
        skill.available=node!=0||skillId>=357;
        const auto name=Name(skillId);std::snprintf(skill.name,sizeof skill.name,"%s",name.c_str());
    }
}
}
