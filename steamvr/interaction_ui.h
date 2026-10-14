#pragma once
#include "interactions.h"
#include "controller_labels.h"
#include <vector>
namespace framevr::interaction {
std::vector<uint8_t> Paint(const Snapshot&,const Output&,int selection,const controllers::Labels& labels={});
}
