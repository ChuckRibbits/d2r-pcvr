#pragma once
#include <cstdint>
#include <array>
#include <algorithm>
#include <span>
#include "../third_party/minhook/src/hde/hde64.h"

namespace melee::native {
// The native server-do table has the same four-argument ABI as the audited
// basic attack and throw. Identify it by both entries; never guess a Smite RVA.
inline uintptr_t SmiteEntry(std::span<const uintptr_t> table,uintptr_t attack,uintptr_t throwing) {
    if(table.size()<151||table[0]||table[1]!=attack||table[3]!=throwing)return 0;
    return table[150];
}
inline bool CallsTarget(std::span<const uint8_t> code,uintptr_t address,uintptr_t target) {
    for(size_t i=0;i<code.size();) {
        // Decode instruction boundaries: an E8 in an immediate is not a call.
        // Padding protects the decoder at the end of the captured function.
        std::array<uint8_t,16> bytes{};
        std::copy_n(code.begin()+i,std::min(size_t(15),code.size()-i),bytes.begin());
        hde64s decoded{};const auto length=hde64_disasm(bytes.data(),&decoded);
        if(!length||(decoded.flags&F_ERROR)||length>code.size()-i)return false;
        if(decoded.opcode==0xe8&&(decoded.flags&F_RELATIVE)&&
           address+i+length+int32_t(decoded.imm.imm32)==target)return true;
        i+=length;
    }
    return false;
}
}
