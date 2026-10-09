#pragma once
#ifdef D2RVR_NATIVE_OPENXR
#include <array>
#include <cstdint>
#include <windows.h>
#include "transport.h"

namespace framevr::profile {
enum Bucket { CameraTick, CameraSync, View, Projection, SkeletonGame, SkeletonMod,
    FoveationPipeline, FoveationTargets, FoveationDraw, HudBind, DepthBind,
    EffectsSetup, EffectsFinish, Count };
struct Counter { uint64_t calls=0,samples=0;double us=0; };
using Counters=std::array<Counter,Count>;
extern thread_local Counters* current;
double Us();
struct Scope {
    Counter* counter=nullptr;double start=0;
    explicit Scope(Bucket bucket,unsigned sampleMask=0) {
        if(!current)return;
        auto& c=(*current)[bucket];++c.calls;
        // Keep sampling phase across pairs; do not repeatedly sample only the
        // first (often cold) callback of each eye. Timings are inclusive.
        static thread_local std::array<uint64_t,Count> cursor{};
        if((++cursor[bucket]&sampleMask)==0){counter=&c;++c.samples;start=Us();}
    }
    ~Scope(){if(counter)counter->us+=Us()-start;}
};
void Request(uint32_t sequence);
void ConfigureGpu(bool enabled);
bool GpuEnabled();
void BeginPair();
void SkipPair();
bool Active();
void BeginEye(unsigned eye);
void FinishPair(double gapMs,double beginMs,double leftMs,double rightMs,double endMs,
                double leftCpuMs,double rightCpuMs,const FrameTiming& xr);
}
#define FRAME_PROFILE(bucket) framevr::profile::Scope frameProfileScope(framevr::profile::bucket)
#define FRAME_PROFILE_SAMPLED(bucket) framevr::profile::Scope frameProfileScope(framevr::profile::bucket,63)
#else
#define FRAME_PROFILE(bucket)
#define FRAME_PROFILE_SAMPLED(bucket)
#endif
