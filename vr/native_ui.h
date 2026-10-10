#pragma once
#include "../steamvr/interactions.h"
namespace D2RL {struct PluginContext;}
namespace nativeui {
void Init(const D2RL::PluginContext*);
void Capture(framevr::interaction::Snapshot&); // UI thread only
}
