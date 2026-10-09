#pragma once
#include "../steamvr/melee_contact.h"
namespace D2RL {struct PluginContext;}
namespace melee {
void Init(const D2RL::PluginContext* context);
void Install();
void Submit(const framevr::melee::Sample& sample);
void Cancel();
void Report();
}
