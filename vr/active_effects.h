#pragma once
#include "../steamvr/active_effects.h"
namespace D2RL {struct PluginContext;}
namespace effectstate {
void Init(const D2RL::PluginContext*);
framevr::effects::Status Sample(uint64_t session); // owning game thread only
}
