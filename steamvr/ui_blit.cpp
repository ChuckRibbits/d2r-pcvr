#include "ui_blit.h"
#include <d3dcompiler.h>
#include <cstring>
#include <climits>
#include <cstdio>

namespace framevr {
using Microsoft::WRL::ComPtr;
bool UiBlit::Initialize(ID3D12Device* device,void (*log)(const char*)) {
    if(pipeline)return true;
    auto fail=[&](){*this={};if(log)log("Frame panels: UI conversion unavailable; native flat menu fallback retained");return false;};
    struct Library {HMODULE module;~Library(){if(module)FreeLibrary(module);}} compiler{LoadLibraryW(L"d3dcompiler_47.dll")};
    const auto compile=compiler.module?reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(compiler.module,"D3DCompile")):nullptr;
    const auto d12=GetModuleHandleW(L"d3d12.dll");
    const auto serialize=d12?reinterpret_cast<decltype(&D3D12SerializeRootSignature)>(GetProcAddress(d12,"D3D12SerializeRootSignature")):nullptr;
    if(!compile||!serialize){if(log)log("Frame panels: missing shader compiler or root serializer");return fail();}
    constexpr char shader[]=R"(
Texture2D<float4> nativeUi : register(t0);
cbuffer Options : register(b0) {uint blackBackground;};
float4 VS(uint id:SV_VertexID):SV_Position {
    return float4(id==2?3:-1,id==1?-3:1,0,1);
}
float4 PS(float4 position:SV_Position):SV_Target {
    float4 c=nativeUi.Load(int3(uint2(position.xy),0));
    float opacity=1-saturate(c.a);
    // D2R's SDR UI shaders encode sRGB before blending into this float target.
    // XR float layers require linear light. Decode the unassociated color,
    // then premultiply again so antialiased text/borders retain their coverage.
    float3 encoded=max(c.rgb,0)/max(opacity,1e-6);
    float3 linearRgb=lerp(pow((encoded+.055)/1.055,2.4),encoded/12.92,encoded<=.04045);
    return float4(opacity>0?linearRgb*opacity:0,blackBackground?1:opacity);
}
)";
    ComPtr<ID3DBlob> vs,ps,errors,signature;
    const auto v=compile(shader,sizeof(shader)-1,"native-ui",nullptr,nullptr,"VS","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vs,&errors);
    const auto p=compile(shader,sizeof(shader)-1,"native-ui",nullptr,nullptr,"PS","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&ps,&errors);
    if(FAILED(v)||FAILED(p)){
        if(log){char b[160];std::snprintf(b,sizeof b,"Frame panels: shader compile failed VS=%08lx PS=%08lx",(unsigned long)v,(unsigned long)p);log(b);if(errors)log(static_cast<const char*>(errors->GetBufferPointer()));}
        return fail();
    }
    D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};
    D3D12_ROOT_PARAMETER parameters[2]{};auto& parameter=parameters[0];parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameter.DescriptorTable={1,&range};parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[1].Constants={0,0,1};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC desc{};desc.NumParameters=2;desc.pParameters=parameters;
    if(FAILED(serialize(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&signature,&errors))||
       FAILED(device->CreateRootSignature(0,signature->GetBufferPointer(),signature->GetBufferSize(),IID_PPV_ARGS(&root)))){
        if(log)log("Frame panels: root signature unavailable");return fail();
    }
    D3D12_GRAPHICS_PIPELINE_STATE_DESC state{};
    state.pRootSignature=root.Get();state.VS={vs->GetBufferPointer(),vs->GetBufferSize()};state.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
    state.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    state.SampleMask=UINT_MAX;state.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;
    state.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;state.RasterizerState.DepthClipEnable=TRUE;
    state.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    state.NumRenderTargets=1;state.RTVFormats[0]=DXGI_FORMAT_R16G16B16A16_FLOAT;state.SampleDesc.Count=1;
    D3D12_DESCRIPTOR_HEAP_DESC heap{};heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;heap.NumDescriptors=1;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    const auto result=device->CreateGraphicsPipelineState(&state,IID_PPV_ARGS(&pipeline));
    if(FAILED(result)) {if(log){char b[128];std::snprintf(b,sizeof b,"Frame panels: pipeline failed %08lx",(unsigned long)result);log(b);}return fail();}
    if(FAILED(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&sourceHeap))))return fail();
    heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if(FAILED(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&targetHeap))))return fail();
    if(log)log("Frame panels: GPU native UI alpha conversion ready; original artwork retained");
    return true;
}
bool UiBlit::Render(ID3D12Device* device,ID3D12GraphicsCommandList* list,ID3D12Resource* source,ID3D12Resource* target,bool blackBackground) {
    if(!pipeline||!source||!target)return false;
    const auto s=source->GetDesc(),t=target->GetDesc();
    if(s.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT||s.Width!=t.Width||s.Height!=t.Height||s.Format!=t.Format||
       s.SampleDesc.Count!=1||t.SampleDesc.Count!=1)return false;
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=s.Format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    device->CreateShaderResourceView(source,&srv,sourceHeap->GetCPUDescriptorHandleForHeapStart());
    device->CreateRenderTargetView(target,nullptr,targetHeap->GetCPUDescriptorHandleForHeapStart());
    auto rtv=targetHeap->GetCPUDescriptorHandleForHeapStart();list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    const D3D12_VIEWPORT vp{0,0,float(t.Width),float(t.Height),0,1};const D3D12_RECT rect{0,0,LONG(t.Width),LONG(t.Height)};
    list->RSSetViewports(1,&vp);list->RSSetScissorRects(1,&rect);
    list->SetGraphicsRootSignature(root.Get());list->SetPipelineState(pipeline.Get());
    ID3D12DescriptorHeap* heaps[]={sourceHeap.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootDescriptorTable(0,sourceHeap->GetGPUDescriptorHandleForHeapStart());
    list->SetGraphicsRoot32BitConstant(1,blackBackground?1:0,0);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
    return true;
}
}
