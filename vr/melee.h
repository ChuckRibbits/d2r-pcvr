#pragma once
#include "../steamvr/melee_contact.h"
namespace D2RL {struct PluginContext;}
namespace melee {
void Init(const D2RL::PluginContext* context);
void Install();
void Submit(const framevr::melee::Sample& sample);
void Cancel();
void Report();
// Owning game thread only; shared with physical arrows. Re-resolves the unit
// from its session/id rather than passing a native pointer across threads.
bool LocalServer(uintptr_t& game,uintptr_t& player);
uintptr_t ServerUnit(uintptr_t game,int type,uint32_t id); // owning game thread only
}
