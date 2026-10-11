#pragma once
#include <d3d12.h>
#include <wrl/client.h>

namespace framevr {
// Convert D2R's SDR sRGB-premultiplied RGB + world-transmittance channel to
// linear premultiplied RGBA for XR. Caller owns synchronization/transitions.
class UiBlit {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> sourceHeap,targetHeap;
public:
    bool Initialize(ID3D12Device* device,void (*log)(const char*));
    bool Render(ID3D12Device*,ID3D12GraphicsCommandList*,ID3D12Resource* source,ID3D12Resource* target,bool blackBackground=false);
};
}
