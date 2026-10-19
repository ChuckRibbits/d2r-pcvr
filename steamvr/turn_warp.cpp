#include "turn_warp.h"
#include "dxgi_views.h"
#include <d3dcompiler.h>
#include <climits>
#include <cstdio>
#include <cstring>

namespace framevr {
using Microsoft::WRL::ComPtr;
bool TurnWarp::Initialize(ID3D12Device* device,DXGI_FORMAT targetFormat,void (*log)(const char*)) {
    if(pipeline)return true;
    auto fail=[&](const char* why){*this={};if(log){char b[160];std::snprintf(b,sizeof b,"Frame OpenXR: turning the eye not drawn unavailable (%s); by turns it goes at its own pose",why);log(b);}return false;};
    struct Library {HMODULE module;~Library(){if(module)FreeLibrary(module);}} compiler{LoadLibraryW(L"d3dcompiler_47.dll")};
    const auto compile=compiler.module?reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(compiler.module,"D3DCompile")):nullptr;
    const auto d12=GetModuleHandleW(L"d3d12.dll");
    const auto serialize=d12?reinterpret_cast<decltype(&D3D12SerializeRootSignature)>(GetProcAddress(d12,"D3D12SerializeRootSignature")):nullptr;
    if(!compile||!serialize)return fail("no shader compiler or root serializer");
    constexpr char shader[]=R"(
Texture2D<float4> drawn : register(t0);
SamplerState edge : register(s0);
// TurnHomography's rows: this frame's image coordinates to the drawn image's
cbuffer Turn : register(b0) {float4 row0;float4 row1;float4 row2;};
struct Pixel {float4 position:SV_Position;float2 uv:TEXCOORD0;};
Pixel VS(uint id:SV_VertexID) {
    Pixel p;p.uv=float2(id==1?2:0,id==2?2:0);
    p.position=float4(p.uv.x*2-1,1-p.uv.y*2,0,1);return p;
}
float4 PS(Pixel p):SV_Target {
    const float3 at=float3(p.uv,1);
    const float3 h=float3(dot(row0.xyz,at),dot(row1.xyz,at),dot(row2.xyz,at));
    if(h.z<=1e-6)return float4(0,0,0,1);   // behind the drawn eye: nothing it saw
    return drawn.SampleLevel(edge,h.xy/h.z,0);   // past its edge: the edge drawn out
}
)";
    ComPtr<ID3DBlob> vs,ps,errors,signature;
    const auto v=compile(shader,sizeof(shader)-1,"turn-warp",nullptr,nullptr,"VS","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vs,&errors);
    const auto p=compile(shader,sizeof(shader)-1,"turn-warp",nullptr,nullptr,"PS","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&ps,&errors);
    if(FAILED(v)||FAILED(p)){if(log&&errors)log(static_cast<const char*>(errors->GetBufferPointer()));return fail("shader compile failed");}
    D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};
    D3D12_ROOT_PARAMETER parameters[2]{};
    parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[0].DescriptorTable={1,&range};parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[1].Constants={0,0,12};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC desc{};desc.NumParameters=2;desc.pParameters=parameters;
    D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;sampler.MaxLOD=D3D12_FLOAT32_MAX;
    desc.NumStaticSamplers=1;desc.pStaticSamplers=&sampler;
    if(FAILED(serialize(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&signature,&errors))||
       FAILED(device->CreateRootSignature(0,signature->GetBufferPointer(),signature->GetBufferSize(),IID_PPV_ARGS(&root))))
        return fail("no root signature");
    D3D12_GRAPHICS_PIPELINE_STATE_DESC state{};
    state.pRootSignature=root.Get();state.VS={vs->GetBufferPointer(),vs->GetBufferSize()};state.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
    state.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    state.SampleMask=UINT_MAX;state.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;
    state.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;state.RasterizerState.DepthClipEnable=TRUE;
    state.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    state.NumRenderTargets=1;state.RTVFormats[0]=targetFormat;state.SampleDesc.Count=1;
    if(FAILED(device->CreateGraphicsPipelineState(&state,IID_PPV_ARGS(&pipeline))))return fail("no pipeline for the eye format");
    D3D12_DESCRIPTOR_HEAP_DESC heap{};heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;heap.NumDescriptors=2;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if(FAILED(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&sourceHeap))))return fail("no descriptor heap");
    heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;heap.NumDescriptors=1;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if(FAILED(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&targetHeap))))return fail("no target heap");
    format=targetFormat;
    if(log)log("Frame OpenXR: eyes by turns - the eye not drawn is turned to each frame's head on the GPU");
    return true;
}
bool TurnWarp::Fits(ID3D12Resource* source,ID3D12Resource* target) const {
    if(!pipeline||!source||!target)return false;
    const auto s=source->GetDesc(),t=target->GetDesc();
    return RenderTargetCompatible(s.Format,format)&&RenderTargetCompatible(t.Format,format)&&s.SampleDesc.Count==1&&t.SampleDesc.Count==1;
}
bool TurnWarp::Render(ID3D12Device* device,ID3D12GraphicsCommandList* list,ID3D12Resource* source,ID3D12Resource* target,const std::array<float,9>& homography,int slot) {
    if(slot<0||slot>1||!Fits(source,target))return false;
    const auto t=target->GetDesc();
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    auto source0=sourceHeap->GetCPUDescriptorHandleForHeapStart();
    auto sourceGpu=sourceHeap->GetGPUDescriptorHandleForHeapStart();
    const UINT step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    source0.ptr+=SIZE_T(step)*slot;sourceGpu.ptr+=UINT64(step)*slot;
    device->CreateShaderResourceView(source,&srv,source0);
    D3D12_RENDER_TARGET_VIEW_DESC rtv{};rtv.Format=format;rtv.ViewDimension=D3D12_RTV_DIMENSION_TEXTURE2D;
    const auto rtvHandle=targetHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateRenderTargetView(target,&rtv,rtvHandle);
    list->SetGraphicsRootSignature(root.Get());
    ID3D12DescriptorHeap* heaps[]={sourceHeap.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootDescriptorTable(0,sourceGpu);
    float rows[12]{};
    for(int r=0;r<3;++r)for(int c=0;c<3;++c)rows[r*4+c]=homography[r*3+c];
    list->SetGraphicsRoot32BitConstants(1,12,rows,0);
    list->OMSetRenderTargets(1,&rtvHandle,FALSE,nullptr);
    const D3D12_VIEWPORT viewport{0,0,float(t.Width),float(t.Height),0,1};const D3D12_RECT rect{0,0,LONG(t.Width),LONG(t.Height)};
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&rect);
    list->SetPipelineState(pipeline.Get());
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
    return true;
}
}
