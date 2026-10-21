#pragma once
#include "../steamvr/interactions.h"
namespace D2RL {struct PluginContext;}
namespace quickitems {
// UI-thread only. The snapshot contains values, never native pointers.
void Init(const D2RL::PluginContext*,const wchar_t* settingsPath=nullptr);
void Capture(framevr::interaction::Snapshot&);
bool Assign(const framevr::interaction::Request&,const framevr::interaction::Snapshot&);
bool Use(const framevr::interaction::Request&,const framevr::interaction::Snapshot&);
}
