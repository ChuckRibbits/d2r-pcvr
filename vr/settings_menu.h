#pragma once
#include "../steamvr/vr_settings.h"
namespace vrsettings {
framevr::settings::Snapshot Read();
bool Submit(const framevr::settings::Request& request);
// Only the settings/update thread reads or writes the INI. XR queues values.
bool Poll(const wchar_t* path);
// Runtime capability, separate from persisted preferences and pending edits.
void SetOfflineDifficultyAvailable(bool available);
}
