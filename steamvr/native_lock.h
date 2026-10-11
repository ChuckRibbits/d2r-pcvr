#pragma once
#ifdef _WIN32
#include <windows.h>
namespace framevr {
// Avoid MinGW winpthreads' process-wide bookkeeping on hot callback locks.
// Nonrecursive, matching std::mutex/std::shared_mutex at these call sites.
class NativeMutex {
    SRWLOCK value=SRWLOCK_INIT;
public:
    void lock(){AcquireSRWLockExclusive(&value);}
    void unlock(){ReleaseSRWLockExclusive(&value);}
    bool try_lock(){return TryAcquireSRWLockExclusive(&value)!=0;}
    void lock_shared(){AcquireSRWLockShared(&value);}
    void unlock_shared(){ReleaseSRWLockShared(&value);}
    bool try_lock_shared(){return TryAcquireSRWLockShared(&value)!=0;}
    NativeMutex()=default;
    NativeMutex(const NativeMutex&)=delete;
    NativeMutex& operator=(const NativeMutex&)=delete;
};
}
#else
#include <shared_mutex>
namespace framevr {using NativeMutex=std::shared_mutex;}
#endif
