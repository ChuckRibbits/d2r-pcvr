#include <windows.h>
#include <D2RLPlugin/api.h>
#include "companions.h"
#include "melee.h"
#include "memory.h"
#include "sigscan.h"
#include "logging.h"
#include <cstdio>
namespace gamestate {uint64_t LocalPlayer();}
namespace companionstate {
namespace {
const D2RL::PluginContext* context=nullptr;
const D2RL::DataTableService* tables=nullptr;
template<class T>bool Read(uintptr_t p,T& value) {
    return p>=0x10000&&p<=UINTPTR_MAX-sizeof value&&framevr::ReadMemory(&value,reinterpret_cast<void*>(p),sizeof value);
}
bool Ready() {
    // Audited from local native code, not from legacy 32-bit unit structures:
    // player data accessor; player pet root; type-list stride; list count/max;
    // node next/id and dead flag. No game function is patched or invoked here.
    constexpr uint8_t data[]={0x48,0x8b,0x43,0x10,0x48,0x83,0xc4,0x20,0x5b,0xc3};
    constexpr uint8_t root[]={0x4c,0x8b,0xbb,0x98,0x00,0x00,0x00};
    constexpr uint8_t stride[]={0x48,0x8b,0xc3,0x48,0xc1,0xe0,0x05,0x48,0x03,0x45,0x00};
    constexpr uint8_t count[]={0x41,0xff,0x4e,0x08,0x41,0xff,0x4e,0x0c};
    constexpr uint8_t node[]={0x48,0x8b,0x58,0x18,0x40,0x84,0xf6,0x75,0x05,0xf6,0x00,0x01,0x75,0x25,0x44,0x8b,0x40,0x04};
    return d2rsig::Check(0x34b27a,data,sizeof data)&&d2rsig::Check(0x4fe9a8,root,sizeof root)&&
        d2rsig::Check(0x50148e,stride,sizeof stride)&&d2rsig::Check(0x5012fd,count,sizeof count)&&
        d2rsig::Check(0x5014f6,node,sizeof node);
}
}
void Init(const D2RL::PluginContext* c){context=c;c->QueryService(&tables);}
framevr::companions::Roster Sample(uint64_t session) {
    using namespace framevr::companions;
    Roster result;uintptr_t game=0,player=0;uint8_t bank=255;
    if(session&&session==gamestate::LocalPlayer()&&tables&&tables->getTable&&Ready()&&
       melee::LocalServer(game,player)&&Read(game+0x106,bank)&&bank<4) {
        D2RL::DataTables::TableView view{};view.structSize=sizeof view;
        if(tables->getTable(context,D2RL::DataTables::Bank(bank),D2RL::DataTables::TableId::PetType,&view)==D2RL::DataTables::Result::Success)
            result=ReadRoster(player,view.rowCount,[](uintptr_t p,auto& value){return Read(p,value);});
    }
    static Roster before;static uint64_t lastSession=0;
    if(result!=before||lastSession!=session) {
        char line[256];int n=std::snprintf(line,sizeof line,"Frame companions: %s",result.known?"roster":"unavailable");
        for(const auto& d:Definitions)if(result.relevant&Bit(d.type))
            n+=std::snprintf(line+n,sizeof line-size_t(n)," %d=%u",d.type,unsigned(result.alive[d.type]));
        framevr::logging::Write(line);before=result;lastSession=session;
    }
    return result;
}
}
