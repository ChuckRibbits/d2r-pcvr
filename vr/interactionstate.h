#pragma once
#include "../steamvr/interactions.h"
namespace D2RL {struct PluginContext;}
namespace interactionstate {
void Init(const D2RL::PluginContext*);
void Capture(); // SDK UI thread, after equipment refresh
framevr::interaction::Snapshot Read();
framevr::interaction::Reply Status();
void Request(const framevr::interaction::Request&);
void Cancel();
}
