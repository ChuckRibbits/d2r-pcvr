#pragma once
#include "vr_settings.h"
#include <vector>
namespace framevr::settings {
std::vector<uint8_t> Paint(const Model& model,Hit hover,const char* runtimeStatus);
}
