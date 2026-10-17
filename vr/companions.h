#pragma once
#include "../steamvr/companions.h"
namespace D2RL {struct PluginContext;}
namespace companionstate {
void Init(const D2RL::PluginContext*);
// Owning game thread only, current offline local server player.
framevr::companions::Roster Sample(uint64_t session);
}
