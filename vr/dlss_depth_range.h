#pragma once
#include <cmath>
#include <cstring>

namespace dlssdepth {
// Only for a live CPU mapping owned by the current native frame builder.
// WriteProcessMemory fails with ERROR_NOACCESS on Proton's Vulkan upload
// mappings even though D3D12 Map guarantees that the CPU can write them.
// Leave the reserved fourth lane intact, without reading write-combined memory.
inline void StoreMappedRange(void* mappedRange, const float range[4]) {
    std::memcpy(mappedRange, range, 3 * sizeof(float));
}
// Frame CB10[53]: {far-near, 1/(far-near), near, reserved}.
// Both LinearizeDepthPass and soft particles use this contract. Adjust the
// shared constants, never only the depth producer. Original camera fields
// remain intact because game simulation/culling also consume those fields.
inline bool PerspectiveRange(const float projection[16], float range[4]) {
    for (int i = 0; i < 16; ++i) if (!std::isfinite(projection[i])) return false;
    if (projection[0] <= 0 || projection[5] <= 0 || projection[10] != 0 ||
        projection[11] != -1 || projection[15] != 0 || projection[14] <= 0) return false;
    if (!std::isfinite(range[0]) || !std::isfinite(range[1]) ||
        !std::isfinite(range[2]) || range[0] <= 0 || range[1] <= 0 ||
        std::fabs(range[0] * range[1] - 1.f) > .001f) return false;
    const float nearPlane = projection[14];
    const float farPlane = range[2] + range[0];
    if (!std::isfinite(farPlane) || farPlane <= nearPlane) return false;
    range[0] = farPlane - nearPlane;
    range[1] = 1.f / range[0];
    range[2] = nearPlane;
    return true;
}
}
