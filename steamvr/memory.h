#pragma once
#include <windows.h>

namespace framevr {
// A page can disappear after VirtualQuery. Ask Wine/Windows to perform the
// guarded read rather than raising an exception inside the game's crash handler.
inline bool ReadMemory(void* dst, const void* src, size_t size) noexcept {
    SIZE_T copied=0;
    return ReadProcessMemory(GetCurrentProcess(),src,dst,size,&copied)&&copied==size;
}
inline bool WriteMemory(void* dst, const void* src, size_t size) noexcept {
    SIZE_T copied=0;
    return WriteProcessMemory(GetCurrentProcess(),dst,src,size,&copied)&&copied==size;
}
}
