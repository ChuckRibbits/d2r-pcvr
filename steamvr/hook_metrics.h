#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#ifdef _WIN32
#include <windows.h>
#else
#include <chrono>
#include <functional>
#include <thread>
#endif

// Bounded, optional all-thread diagnostics. Each recording thread owns a slot;
// no allocation, shared lock, logging or server call on the callback path.
// Counters are atomic so the render thread can snapshot workers without races.
// Values are monotonic: starting another capture never clears a live counter.
namespace framevr::hookmetrics {
enum Kind { CameraTick, CameraSync, View, Projection, SkeletonGame, SkeletonMod,
    FoveationPipeline, FoveationTargets, FoveationDraw, HudBind, DepthBind,
    EffectsSetup, EffectsFinish, MemoryRead, MemoryWrite, PortalView, PortalCenter,
    UiWorldBind, TraceDraw, InputTick, FoveationLookup, Count };
inline constexpr const char* Names[Count]={"camera-game","camera-sync","view","projection","skeleton-game","skeleton-mod",
    "vrs-pipeline","vrs-targets","vrs-draw","hud-bind","depth-bind","fx-setup","fx-finish",
    "memory-read","memory-write","portal-view","portal-center","ui-world-bind","trace-draw","input-tick","vrs-lookup"};
inline constexpr unsigned MaxThreads=128;
struct Counter {uint64_t calls=0,samples=0,nanos=0,bytes=0;};
struct AtomicCounter {std::atomic<uint64_t> calls{0},samples{0},nanos{0},bytes{0};};
struct alignas(64) ThreadSlot {std::atomic<uint64_t> id{0};std::array<AtomicCounter,Count> counters;};
struct ThreadSnapshot {uint64_t id=0;std::array<Counter,Count> counters;};
using Snapshot=std::array<ThreadSnapshot,MaxThreads>;
inline std::array<ThreadSlot,MaxThreads> slots;
inline std::atomic<unsigned> allocated{0};
inline std::atomic<uint64_t> epoch{0};
inline std::atomic<bool> overflow{false};
inline uint64_t Nanos() {
#ifdef _WIN32
    static const double scale=[](){LARGE_INTEGER f;QueryPerformanceFrequency(&f);return 1e9/double(f.QuadPart);}();
    LARGE_INTEGER n;QueryPerformanceCounter(&n);return uint64_t(n.QuadPart*scale);
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}
inline ThreadSlot* Thread() {
    static thread_local ThreadSlot* slot=[]()->ThreadSlot* {
        const auto index=allocated.fetch_add(1,std::memory_order_relaxed);
        if(index>=MaxThreads){overflow.store(true,std::memory_order_relaxed);return nullptr;}
#ifdef _WIN32
        const uint64_t id=GetCurrentThreadId();
#else
        const uint64_t id=std::hash<std::thread::id>{}(std::this_thread::get_id());
#endif
        auto* result=&slots[index];result->id.store(id?id:1,std::memory_order_release);return result;
    }();
    return slot;
}
inline Snapshot Read() {
    Snapshot result{};
    for(unsigned t=0;t<MaxThreads;++t) {
        auto& s=slots[t];auto& r=result[t];r.id=s.id.load(std::memory_order_acquire);if(!r.id)continue;
        for(unsigned b=0;b<Count;++b) {
            const auto& c=s.counters[b];r.counters[b]={c.calls.load(std::memory_order_relaxed),c.samples.load(std::memory_order_relaxed),
                c.nanos.load(std::memory_order_relaxed),c.bytes.load(std::memory_order_relaxed)};
        }
    }
    return result;
}
inline Snapshot Start(uint64_t generation) {
    epoch.store(0,std::memory_order_release);
    auto baseline=Read();epoch.store(generation,std::memory_order_release);return baseline;
}
inline void Stop(){epoch.store(0,std::memory_order_release);}
struct Scope {
    AtomicCounter* counter=nullptr;uint64_t generation=0,start=0;
    explicit Scope(Kind kind,unsigned sampleMask=63,uint64_t bytes=0) {
        generation=epoch.load(std::memory_order_acquire);if(!generation)return;
        auto* thread=Thread();if(!thread)return;
        auto& c=thread->counters[kind];c.calls.fetch_add(1,std::memory_order_relaxed);
        if(bytes)c.bytes.fetch_add(bytes,std::memory_order_relaxed);
        static thread_local std::array<uint64_t,Count> cursor{};
        if((++cursor[kind]&sampleMask)==0){counter=&c;start=Nanos();}
    }
    ~Scope() {
        if(counter&&epoch.load(std::memory_order_acquire)==generation) {
            const auto elapsed=Nanos()-start;
            counter->nanos.fetch_add(elapsed,std::memory_order_relaxed);
            counter->samples.fetch_add(1,std::memory_order_relaxed);
        }
    }
    Scope(const Scope&)=delete;
};
}
