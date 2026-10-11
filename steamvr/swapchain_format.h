#pragma once
#include <dxgiformat.h>
#include <algorithm>
#include <cstdint>
#include <span>

namespace framevr {
// D2R's SDR backbuffer and uploaded UI artwork contain display-encoded bytes.
// CopyResource preserves those bytes; OpenXR must decode them as sRGB even if
// the runtime also supports a linear UNORM swapchain. Do not silently submit
// encoded SDR bytes as linear. Other (including float) formats stay unchanged.
inline DXGI_FORMAT CopiedSwapchainFormat(DXGI_FORMAT source,std::span<const int64_t> supported) {
    auto requested=source;
    if(source==DXGI_FORMAT_R8G8B8A8_UNORM)requested=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    else if(source==DXGI_FORMAT_B8G8R8A8_UNORM)requested=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    return std::find(supported.begin(),supported.end(),int64_t(requested))!=supported.end()?requested:DXGI_FORMAT_UNKNOWN;
}
}
