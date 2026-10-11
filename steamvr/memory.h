#pragma once
#include <windows.h>
#include "hook_metrics.h"

namespace framevr {
// A page can disappear after VirtualQuery. Ask Wine/Windows to perform the
// guarded read rather than raising an exception inside the game's crash handler.
inline bool ReadMemory(void* dst, const void* src, size_t size) noexcept {
    hookmetrics::Scope measured(hookmetrics::MemoryRead,63,size);
    SIZE_T copied=0;
    return ReadProcessMemory(GetCurrentProcess(),src,dst,size,&copied)&&copied==size;
}
inline bool WriteMemory(void* dst, const void* src, size_t size) noexcept {
    hookmetrics::Scope measured(hookmetrics::MemoryWrite,63,size);
    SIZE_T copied=0;
    // Copy into an already writable buffer in THIS process. Wine's current-
    // process read path guards both source and destination inside ntdll and
    // performs the copy locally; WriteProcessMemory serializes via wineserver.
    // No permission changes or code patching: inaccessible destinations fail.
    // This also avoids an application-level access violation reaching D2R's
    // crash handler before a C++/SEH guard could handle it.
    return ReadProcessMemory(GetCurrentProcess(),src,dst,size,&copied)&&copied==size;
}
}
