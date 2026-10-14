#pragma once
#include <cstdint>
namespace nativebelt {
// UI thread only, after the caller revalidates the exact captured belt item.
bool Drink(int column,uint64_t session);
}
