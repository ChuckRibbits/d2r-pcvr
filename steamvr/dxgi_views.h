#pragma once
#include <dxgiformat.h>

namespace framevr {
// OpenXR can expose typeless resources for a typed swapchain format. The PSO
// and its views must still use the typed format requested at creation.
inline bool RenderTargetCompatible(DXGI_FORMAT resource,DXGI_FORMAT view) {
    if(resource==view)return true;
    switch(resource) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        return view==DXGI_FORMAT_R8G8B8A8_UNORM||view==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        return view==DXGI_FORMAT_B8G8R8A8_UNORM||view==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        return view==DXGI_FORMAT_R16G16B16A16_FLOAT;
    default:return false;
    }
}
}
