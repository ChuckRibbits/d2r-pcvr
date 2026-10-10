#include <windows.h>
#include <D2RLPlugin/api.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <cstddef>
#include <map>
#include <string>
#include "native_ui.h"
#include "native_ui_signatures.h"
#include "sigscan.h"
#include "memory.h"
#include "combat.h"
#include "logging.h"
#include "controller_bindings.h"
#include "skill_slots.h"
#include "spell_aim.h"
#include "panel_input.h"
namespace nativeui {
namespace {
const D2RL::PluginContext* context=nullptr;
const D2RL::LocalizationService* strings=nullptr;
std::map<int,std::string> names;
bool ready=false;
bool bindingReady=false;
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
    CHECK(ControllerBank);CHECK(ControllerOwner);CHECK(SkillLevelLayout);
#undef CHECK
    ready=true;framevr::logging::Write("Frame interactions: controller-only assignments and resource readers verified (independent of mouse/keyboard mode)");return true;
}
bool BindingReady() {
    if(bindingReady)return true;
    using namespace abi;
#define CHECK(x) if(!d2rsig::Check(x##Rva,x##Sig,sizeof x##Sig))return false
    CHECK(AssignSkill);CHECK(AssignSkillArgs);CHECK(AssignSkillBank);CHECK(ClearSkill);CHECK(ClearSkillWrite);
    CHECK(SkillPositive);CHECK(SkillPositiveLevel);CHECK(SkillLeftFlag);
#undef CHECK
    bindingReady=true;framevr::logging::Write("Frame skill editor: native assignment, clear and catalog readers verified");return true;
}
int Slot(int wheel) {
    if(wheel<0||wheel>=8)return -1;
    return reinterpret_cast<int(*)(int,bool)>(d2rsig::Addr(abi::ButtonSlotRva))(
        framevr::skills::Buttons(wheel),framevr::skills::Alternate(wheel));
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
    const int bank=reinterpret_cast<int(*)()>(d2rsig::Addr(abi::ControllerBankRva))();
    if(bank!=0&&bank!=16)return;
    s.bindingBank=bank;
    for(int i=0;i<8;++i) {
        const int slot=Slot(i);
        if(slot<0||slot>=16)continue;
        auto& skill=s.skills[i];
        framevr::interaction::ControllerBinding binding;
        if(!framevr::interaction::ReadControllerBinding(d2rsig::Addr(abi::ControllerTableRva),bank,slot,
            [](uintptr_t p,auto& value){return Read(p,value);},binding))continue;
        int skillId=binding.skill;
        // Preserve the native controller getter's combined Town Portal shortcut:
        // use a scroll when its base level is positive, otherwise the tome.
        if(skillId==359) {
            const auto scroll=reinterpret_cast<uintptr_t(*)(uintptr_t,int,int)>(d2rsig::Addr(abi::SkillRva))(player,219,-1);
            int level=0;if(scroll&&!Read(scroll+0x48,level))continue;
            skillId=scroll&&level>0?219:220;
        }
        skill.id=skillId;skill.owner=binding.owner;skill.known=true;
        if(skillId==-1){std::snprintf(skill.name,sizeof skill.name,"Unassigned");continue;}
        const int owner=binding.owner;
        const auto node=reinterpret_cast<uintptr_t(*)(uintptr_t,int,int)>(d2rsig::Addr(abi::SkillRva))(player,skillId,owner);
        // Presence is availability of the assignment, not a mana/cooldown
        // guarantee. Those gates remain in the native casting path.
        skill.available=node!=0||skillId>=357;
        // Same checked skill-node -> compiled Skills row used by the catalog.
        // Offsets independently audited across all 429 local skill records.
        uintptr_t row=0;int16_t rowId=-1,missile=-1,serverFunction=0;uint32_t flags=0;
        if(node&&Read(node,row)&&Read(row,rowId)&&rowId==skillId&&
           Read(row+0x24,flags)&&Read(row+0x4e,serverFunction)&&Read(row+0x66,missile))
            skill.directed=framevr::spellaim::Directed(flags,missile,serverFunction);
        const auto name=Name(skillId);std::snprintf(skill.name,sizeof skill.name,"%s",name.c_str());
    }
    if(BindingReady()) {
        uintptr_t node=0;bool valid=Read(skillList,node);
        unsigned visited=0;
        for(;valid&&node&&visited<256;++visited) {
            struct Node {uintptr_t row,next;uint8_t pad[0x30];int level,bonus,base,owner;} n{};
            static_assert(offsetof(Node,owner)==0x4c);
            struct Row {int16_t id;uint8_t pad[0x22],flags[4];} row{};
            if(!Read(node,n)||!Read(n.row,row)||row.id<0||row.id>4095||n.next==node){valid=false;break;}
            // Native compiled Skills row flags; checked against the local
            // skills.bin and source table: passive +0x24/0x10, leftskill +0x27/0x40.
            const bool positive=reinterpret_cast<bool(*)(uintptr_t,uintptr_t)>(d2rsig::Addr(abi::SkillPositiveRva))(player,node);
            const int choice=framevr::interaction::CatalogSkillId(row.id);
            const int owner=choice==359?-1:n.owner;
            const bool duplicate=std::any_of(s.catalog.begin(),s.catalog.begin()+s.skillCount,[&](const auto& c){return c.id==choice&&c.owner==owner;});
            if(choice>=0&&!(row.flags[0]&0x10)&&positive&&!duplicate) {
                if(s.skillCount==s.catalog.size()){valid=false;break;}
                auto& option=s.catalog[s.skillCount++];option.id=choice;option.owner=owner;
                option.level=n.level+n.bonus;option.known=option.available=true;
                const auto name=Name(choice);std::snprintf(option.name,sizeof option.name,"%s",name.c_str());
            }
            node=n.next;
        }
        if(node)valid=false;
        s.bindingsEditable=valid&&std::all_of(s.skills.begin(),s.skills.end(),[](const auto& skill){return skill.known;});
        if(!valid){s.catalog={};s.skillCount=0;}
        else std::sort(s.catalog.begin(),s.catalog.begin()+s.skillCount,[](const auto& a,const auto& b){
            const int order=std::strcmp(a.name,b.name);return order?order<0:a.owner<b.owner;
        });
    }
    static uint64_t lastSession=0;static int lastBank=-1;static std::array<int,8> lastIds{};static unsigned lastDirected=0;
    unsigned directed=0;std::array<int,8> ids{};
    for(int i=0;i<8;++i){ids[i]=s.skills[i].known?s.skills[i].id:-2;if(s.skills[i].directed)directed|=1u<<i;}
    if(lastSession!=s.session||lastBank!=bank||lastIds!=ids||lastDirected!=directed) {
        char line[200];std::snprintf(line,sizeof line,"Frame wheel assignments: controller bank=%d skills=%d,%d,%d,%d,%d,%d,%d,%d directed=0x%02X",
            bank,ids[0],ids[1],ids[2],ids[3],ids[4],ids[5],ids[6],ids[7],directed);
        framevr::logging::Write(line);lastSession=s.session;lastBank=bank;lastIds=ids;lastDirected=directed;
    }
}
bool Assign(const framevr::interaction::Request& r,const framevr::interaction::Snapshot& current) {
    if(!Ready()||!BindingReady()||!framevr::interaction::CanBind(current,r,GetTickCount64()))return false;
    const int slot=Slot(r.index);
    const int bank=reinterpret_cast<int(*)()>(d2rsig::Addr(abi::ControllerBankRva))();
    if(slot<0||slot>=16||bank!=r.bank)return false;
    bool left=false;
    if(r.skill>=0) {
        const int index=reinterpret_cast<int(*)()>(d2rsig::Addr(abi::LocalIndexRva))();
        if(index<0||index>=8)return false;
        const auto player=reinterpret_cast<uintptr_t(*)(int)>(d2rsig::Addr(abi::ClientPlayerRva))(index);
        const auto lookup=reinterpret_cast<uintptr_t(*)(uintptr_t,int,int)>(d2rsig::Addr(abi::SkillRva));
        auto node=lookup(player,r.skill==359?219:r.skill,r.skillOwner);
        if(r.skill==359) {
            int level=0;
            if(!node||!Read(node+0x48,level)||level<=0)node=lookup(player,220,-1);
        }
        uintptr_t row=0;uint8_t flag=0;
        if(!node||!Read(node,row)||!Read(row+0x27,flag))return false;
        left=(flag&0x40)!=0||r.skill==357||r.skill==358||r.skill==370;
    }
    struct Args {int slot,skill,owner;bool left;} args{slot,r.skill,r.skillOwner,left};
    // The native setter chooses its table through the controller getter. Scope
    // that choice to this synchronous UI-thread operation; do not alter the
    // user's desktop input mode or write to their save/skill records ourselves.
    if(!panelinput::ControllerAction(+[](void* data){
        const auto& a=*static_cast<Args*>(data);
        if(a.skill<0)reinterpret_cast<void(*)(int)>(d2rsig::Addr(abi::ClearSkillRva))(a.slot);
        else reinterpret_cast<void(*)(int,bool,int,int)>(d2rsig::Addr(abi::AssignSkillRva))(a.slot,a.left,a.skill,a.owner);
    },&args))return false;
    framevr::interaction::ControllerBinding value;
    const bool applied=framevr::interaction::ReadControllerBinding(d2rsig::Addr(abi::ControllerTableRva),bank,slot,
        [](uintptr_t p,auto& v){return Read(p,v);},value)&&value.skill==r.skill&&value.owner==r.skillOwner;
    char line[160];std::snprintf(line,sizeof line,"Frame skill editor: slot=%d bank=%d skill=%d native readback=%s",r.index,bank,r.skill,applied?"confirmed":"refused");
    framevr::logging::Write(line);return applied;
}
}
