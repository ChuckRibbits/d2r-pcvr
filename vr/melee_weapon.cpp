// D2R 3.3.93787: scope the native weapon selector during one physical attack.
// Native stat-list merging, hit rolls, damage, procs and durability remain in
// the game. No equipment, skill selection or persistent item data is replaced.
#include "melee_weapon.h"
#include "melee_weapon_signatures.h"
#include "sigscan.h"
#include "memory.h"
#include "logging.h"
#include <atomic>

namespace meleeweapon {
namespace {
using namespace abi;
using WeaponFn=uintptr_t(*)(uintptr_t);
WeaponFn originalWeapon=nullptr;
std::atomic<bool> ready{false};
thread_local uintptr_t scopedPlayer=0,scopedWeapon=0;
uintptr_t CurrentWeapon(uintptr_t player) {
    return player&&player==scopedPlayer&&scopedWeapon?scopedWeapon:originalWeapon(player);
}
template<class T> bool Read(uintptr_t p,T& out) {
    return p>0x10000&&framevr::ReadMemory(&out,reinterpret_cast<void*>(p),sizeof out);
}
void Merge(uintptr_t player) {
    reinterpret_cast<void(*)(uintptr_t,int)>(d2rsig::Addr(MergeRva))(player,0);
}
}
void Install() {
    if(ready.load())return;
    for(const auto& guard:Guards)if(!d2rsig::Check(guard.rva,guard.bytes,guard.size))return;
    if(!d2rsig::Hook(WeaponRva,WeaponSig,sizeof WeaponSig,reinterpret_cast<void*>(&CurrentWeapon),reinterpret_cast<void**>(&originalWeapon)))return;
    ready=true;
    framevr::logging::Write("Frame dual wield: scoped native weapon selection ready; Barbarian physical hands enabled");
}
bool Ready(){return ready.load();}
uintptr_t Select(uintptr_t player,int hand,const framevr::dualwield::Equipment& e) {
    uint32_t hero[3]{};
    if(!Ready()||hand<0||hand>1||!Read(player,hero)||hero[0]!=0||!framevr::dualwield::Eligible(int(hero[1]),e))return 0;
    const auto inventory=reinterpret_cast<WeaponFn>(d2rsig::Addr(InventoryRva))(player);
    uint32_t magic=0;if(!Read(inventory,magic)||magic!=0x01020304)return 0;
    uintptr_t item[2]{};uint32_t ids[2]{};
    for(int side=0;side<2;++side) {
        item[side]=reinterpret_cast<uintptr_t(*)(uintptr_t,uint8_t)>(d2rsig::Addr(CompositeRva))(inventory,uint8_t(5+side));
        uint32_t header[3]{};
        if(!Read(item[side],header)||header[0]!=4)return 0;
        ids[side]=header[2];
        if(!reinterpret_cast<int(*)(uintptr_t,int)>(d2rsig::Addr(TypeRva))(item[side],45)||
           !reinterpret_cast<int(*)(uintptr_t)>(d2rsig::Addr(UsableRva))(item[side]))return 0;
    }
    return framevr::dualwield::SamePair(ids[0],ids[1],e)?item[hand]:0;
}
Scope::Scope(uintptr_t p,uintptr_t weapon):player(weapon?p:0),previousPlayer(scopedPlayer),previousWeapon(scopedWeapon) {
    if(player) {
        scopedPlayer=player;scopedWeapon=weapon;
        // The basic attack checks accuracy before filling damage. Refresh the
        // selected weapon's native stat lists before either calculation.
        Merge(player);
    }
}
Scope::~Scope() {
    if(player) {
        scopedPlayer=previousPlayer;scopedWeapon=previousWeapon;
        // Restore the game's normal selection (or an enclosing scope).
        Merge(player);
    }
}
}
