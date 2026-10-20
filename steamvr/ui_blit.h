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
    bool Render(ID3D12Device*,ID3D12GraphicsCommandList*,ID3D12Resource* source,ID3D12Resource* target,bool blackBackground=false,bool squareMap=false,uint32_t mapFlags=0,float bearing=0,float wallAlpha=1.f);
};
}
