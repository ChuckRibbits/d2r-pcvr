#pragma once
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

namespace framevr {
// Convert D2R's SDR sRGB-premultiplied RGB + world-transmittance channel to
// linear premultiplied RGBA for XR. Caller owns synchronization/transitions.
class UiBlit {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline,findPipeline;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> sourceHeap,targetHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> hero;   // 1x1: the map's hero marker, found each frame (square map)
public:
    bool Initialize(ID3D12Device* device,void (*log)(const char*));
    // mapFlags (square map only): 1 heading arrow at bearing (radians, clockwise from map-up), 2 clear background, walls at wallAlpha, 4 the map turns with you
    // zoom (square map only, >= 1): the map that much bigger about the hero; 1 fits it whole
    // mapSpan (square map only, > 0; Map view = Full map): a square of that many source pixels across the whole square around the hero's X, the arrow sized from the X; 0: the fit, zoomed
    bool Render(ID3D12Device*,ID3D12GraphicsCommandList*,ID3D12Resource* source,ID3D12Resource* target,bool blackBackground=false,bool squareMap=false,uint32_t mapFlags=0,float bearing=0,float wallAlpha=1.f,float zoom=1.f,float mapSpan=0.f);
};
}
