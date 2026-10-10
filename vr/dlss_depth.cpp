#include "dlss_depth.h"
#include "dlss_depth_range.h"
#include "sigscan.h"
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#ifdef D2RVR_NATIVE_OPENXR
#include "memory.h"
#endif

namespace dlssdepth {
namespace {
void (*logger)(const char*) = nullptr;
bool (*wanted)() = nullptr;
using BuildFrameFn = void (*)(void* renderer, void* frame, void* camera, float time);
BuildFrameFn original = nullptr;
bool installed = false;
std::atomic<bool> reported{false};
// D2R 3.3.93787: frame-constant allocation/writer. Verified call sites pass
// renderer, frame context, camera, float time. The fresh mapped allocation is
// consumed only after this function returns. No GPU resource or wait is added.
constexpr uint64_t BuildRva = 0xE3B590;
constexpr uint8_t BuildSig[] = {0x4c,0x8b,0xdc,0x53,0x48,0x81,0xec,0x00,0x02,0x00,0x00};
constexpr uint8_t CameraBindingSig[] = {0x48,0x8d,0x8b,0x00,0x1a,0x01,0x00};
constexpr uint8_t MapSig[] = {0x48,0x8b,0xbe,0xb8,0x1b,0x01,0x00};
constexpr uint8_t RangeSig[] = {0xf3,0x41,0x0f,0x10,0x94,0x24,0x60,0x01,0x00,0x00,
    0xf3,0x41,0x0f,0x10,0x8c,0x24,0x64,0x01,0x00,0x00};
constexpr uint8_t StoreSig[] = {0x0f,0x11,0x87,0x50,0x03,0x00,0x00};
bool Read(void* dst, const void* src, size_t size) {
#ifdef D2RVR_NATIVE_OPENXR
    return framevr::ReadMemory(dst, src, size);
#else
    __try { std::memcpy(dst, src, size); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
void BuildFrame(void* renderer, void* frame, void* camera, float time) {
    original(renderer, frame, camera, time);
    if (!wanted || !wanted() || !renderer || !camera) return;
    float projection[16], range[4]; uintptr_t constants = 0, cameraConstants = 0;
    // Use the projection actually uploaded for native shader consumers.
    // Check the consumer's uploaded projection rather than assuming the
    // camera object's cached matrix still represents the current VR pass.
    // Camera CB3[13..16] is the unjittered projection (byte offset 0xD0).
    if (!Read(&cameraConstants, static_cast<uint8_t*>(renderer) + 0x11a08, sizeof cameraConstants) ||
        cameraConstants < 0x10000 ||
        !Read(projection, reinterpret_cast<void*>(cameraConstants + 0xd0), sizeof projection) ||
        !Read(&constants, static_cast<uint8_t*>(renderer) + 0x11bb8, sizeof constants) ||
        constants < 0x10000 || !Read(range, reinterpret_cast<void*>(constants + 0x350), sizeof range)) return;
    if (!PerspectiveRange(projection, range)) return;
    // Preserve the reserved fourth lane. This writes only this frame's newly
    // allocated CPU upload storage, before any native pass is submitted.
    // The verified native builder just wrote this same mapped allocation;
    // its owner cannot recycle it before this hook returns to the caller.
    StoreMappedRange(reinterpret_cast<void*>(constants + 0x350), range);
    if (!reported.exchange(true) && logger) {
        char line[192]; std::snprintf(line, sizeof line,
            "VR depth: shared frame range active near=%.4f far=%.4f; native depth and particle decoding agree",
            range[2], range[2] + range[0]); logger(line);
    }
}
}
void Install(void (*log)(const char*), bool (*active)()) {
    if (installed) return;
    logger = log; wanted = active;
    if (!d2rsig::Check(BuildRva, BuildSig, sizeof BuildSig) ||
        !d2rsig::Check(0xE34FF3, CameraBindingSig, sizeof CameraBindingSig) ||
        !d2rsig::Check(0xE3B63D, MapSig, sizeof MapSig) ||
        !d2rsig::Check(0xE3BE07, RangeSig, sizeof RangeSig) ||
        !d2rsig::Check(0xE3BE56, StoreSig, sizeof StoreSig)) {
        static bool told = false;
        if (!told && logger) { told = true; logger("VR depth: frame writer not verified; leaving native depth pipeline unchanged"); }
        return;
    }
    installed = d2rsig::Hook(BuildRva, BuildSig, sizeof BuildSig, reinterpret_cast<void*>(&BuildFrame), reinterpret_cast<void**>(&original));
    if (installed && logger) logger("VR depth: verified shared frame-range hook installed; no depth shader replacement");
}
}
