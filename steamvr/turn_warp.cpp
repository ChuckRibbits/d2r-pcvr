#include "turn_warp.h"
#include "dxgi_views.h"
#include <d3dcompiler.h>
#include <climits>
#include <cstdio>
#include <cstring>

namespace framevr {
using Microsoft::WRL::ComPtr;
namespace {
constexpr char Shader[]=R"(
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
// The grid with the scene's depth (TurnDepth).
constexpr char DepthShader[]=R"(
Texture2D<float4> drawn : register(t0);
Texture2D<float> drawnDepth : register(t1);
SamplerState edge : register(s0);
cbuffer Move : register(b0) {
    float4 move0;float4 move1;float4 move2;   // TurnMove's rows
    float4 drawnTan;   // tangents of the drawn eye's FOV: left, right, down, up
    float4 nowTan;     // this frame's
    float4 depthInfo;  // near plane, the depth's drawn part x, y
    uint4 grid;        // cells across, down
    float4 carry0;float4 carry1;float4 carry2;   // the carried's rows (TurnDepth::carry)
    float4 reach;      // nearer than x: carried; farther than y: the world's
};
struct Pixel {float4 position:SV_Position;float2 uv:TEXCOORD0;};
static const float2 Corner[6]={float2(0,0),float2(1,0),float2(0,1),float2(0,1),float2(1,0),float2(1,1)};
static const float Ahead=1e-3;   // nearer than this to this frame's eye: cut off
Pixel VS(uint id:SV_VertexID) {
    const uint cell=id/6;
    const float2 uv=(float2(cell%grid.x,cell/grid.x)+Corner[id%6])/float2(grid.xy);
    // The nearest of the texels round the point (reverse-Z: the largest), so
    // an edge stays with what is in front; the sky (0) far, not infinite.
    const float4 d4=drawnDepth.GatherRed(edge,uv*depthInfo.yz);
    const float d=max(max(max(d4.x,d4.y),max(d4.z,d4.w)),1e-6);
    const float away=depthInfo.x/d;
    const float3 p=float3(lerp(drawnTan.x,drawnTan.y,uv.x),lerp(drawnTan.w,drawnTan.z,uv.y),-1)*away;
    const float3 world=float3(dot(move0.xyz,p)+move0.w,dot(move1.xyz,p)+move1.w,dot(move2.xyz,p)+move2.w);
    const float3 carried=float3(dot(carry0.xyz,p)+carry0.w,dot(carry1.xyz,p)+carry1.w,dot(carry2.xyz,p)+carry2.w);
    const float3 q=lerp(carried,world,saturate((away-reach.x)/max(reach.y-reach.x,1e-6)));
    // this frame's projection, its depth reversed: nearer is larger
    const float w=-q.z;
    Pixel o;o.uv=uv;
    o.position=float4(2*(q.x-nowTan.x*w)/(nowTan.y-nowTan.x)-w,w-2*(nowTan.w*w-q.y)/(nowTan.w-nowTan.z),Ahead,w);
    return o;
}
float4 PS(Pixel p):SV_Target {return drawn.SampleLevel(edge,p.uv,0);}
)";
constexpr UINT DepthConstants=44;
}
bool TurnWarp::Initialize(ID3D12Device* device,DXGI_FORMAT targetFormat,void (*log)(const char*)) {
    if(pipeline)return true;
    auto fail=[&](const char* why){*this={};if(log){char b[160];std::snprintf(b,sizeof b,"Frame OpenXR: turning the eye not drawn unavailable (%s); by turns it goes at its own pose",why);log(b);}return false;};
    struct Library {HMODULE module;~Library(){if(module)FreeLibrary(module);}} compiler{LoadLibraryW(L"d3dcompiler_47.dll")};
    const auto compile=compiler.module?reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(compiler.module,"D3DCompile")):nullptr;
    const auto d12=GetModuleHandleW(L"d3d12.dll");
    const auto serialize=d12?reinterpret_cast<decltype(&D3D12SerializeRootSignature)>(GetProcAddress(d12,"D3D12SerializeRootSignature")):nullptr;
    if(!compile||!serialize)return fail("no shader compiler or root serializer");
    ComPtr<ID3DBlob> vs,ps,errors,signature;
    const auto v=compile(Shader,sizeof(Shader)-1,"turn-warp",nullptr,nullptr,"VS","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vs,&errors);
    const auto p=compile(Shader,sizeof(Shader)-1,"turn-warp",nullptr,nullptr,"PS","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&ps,&errors);
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
    // Each eye's slot: its image, then its depth (the depth pass reads both).
    D3D12_DESCRIPTOR_HEAP_DESC heap{};heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;heap.NumDescriptors=4;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if(FAILED(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&sourceHeap))))return fail("no descriptor heap");
    heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;heap.NumDescriptors=1;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if(FAILED(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&targetHeap))))return fail("no target heap");
    format=targetFormat;
    if(log)log("Frame OpenXR: eyes by turns - the eye not drawn is turned to each frame's head on the GPU");
    // The depth pass is optional: without it By turns draws pairs while the hero moves.
    const auto depthFail=[&](const char* why){
        depthRoot.Reset();depthPipeline.Reset();depthHeap.Reset();
        if(log){char b[160];std::snprintf(b,sizeof b,"Frame OpenXR: by turns - moving the eye with the scene depth unavailable (%s)",why);log(b);}
        return true;
    };
    ComPtr<ID3DBlob> dvs,dps;errors.Reset();signature.Reset();
    const auto dv=compile(DepthShader,sizeof(DepthShader)-1,"turn-depth",nullptr,nullptr,"VS","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&dvs,&errors);
    const auto dp=compile(DepthShader,sizeof(DepthShader)-1,"turn-depth",nullptr,nullptr,"PS","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&dps,&errors);
    if(FAILED(dv)||FAILED(dp)){if(log&&errors)log(static_cast<const char*>(errors->GetBufferPointer()));return depthFail("shader compile failed");}
    D3D12_DESCRIPTOR_RANGE both{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0};
    D3D12_ROOT_PARAMETER depthParameters[2]{};
    depthParameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    depthParameters[0].DescriptorTable={1,&both};depthParameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
    depthParameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    depthParameters[1].Constants={0,0,DepthConstants};depthParameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
    D3D12_ROOT_SIGNATURE_DESC depthDesc=desc;depthDesc.pParameters=depthParameters;
    D3D12_STATIC_SAMPLER_DESC depthSampler=sampler;depthSampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;depthDesc.pStaticSamplers=&depthSampler;
    if(FAILED(serialize(&depthDesc,D3D_ROOT_SIGNATURE_VERSION_1,&signature,&errors))||
       FAILED(device->CreateRootSignature(0,signature->GetBufferPointer(),signature->GetBufferSize(),IID_PPV_ARGS(&depthRoot))))
        return depthFail("no root signature");
    state.pRootSignature=depthRoot.Get();state.VS={dvs->GetBufferPointer(),dvs->GetBufferSize()};state.PS={dps->GetBufferPointer(),dps->GetBufferSize()};
    state.DepthStencilState.DepthEnable=TRUE;state.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;
    state.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_GREATER;state.DSVFormat=DXGI_FORMAT_D32_FLOAT;
    if(FAILED(device->CreateGraphicsPipelineState(&state,IID_PPV_ARGS(&depthPipeline))))return depthFail("no pipeline");
    heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    if(FAILED(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&depthHeap))))return depthFail("no depth heap");
    if(log)log("Frame OpenXR: by turns - the eye not drawn can be moved with the scene depth");
    return true;
}
bool TurnWarp::Fits(ID3D12Resource* source,ID3D12Resource* target) const {
    if(!pipeline||!source||!target)return false;
    const auto s=source->GetDesc(),t=target->GetDesc();
    return RenderTargetCompatible(s.Format,format)&&RenderTargetCompatible(t.Format,format)&&s.SampleDesc.Count==1&&t.SampleDesc.Count==1;
}
bool TurnWarp::Render(ID3D12Device* device,ID3D12GraphicsCommandList* list,ID3D12Resource* source,ID3D12Resource* target,
                      const std::array<float,9>& homography,int slot,const TurnDepth* depth) {
    if(slot<0||slot>1||!Fits(source,target))return false;
    const auto t=target->GetDesc();
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    auto source0=sourceHeap->GetCPUDescriptorHandleForHeapStart();
    auto sourceGpu=sourceHeap->GetGPUDescriptorHandleForHeapStart();
    const UINT step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    source0.ptr+=SIZE_T(step)*2*slot;sourceGpu.ptr+=UINT64(step)*2*slot;
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
    if(!depth||!depth->depth||!depthPipeline||depth->nearPlane<=0)return true;
    // The grid over what the turn drew.
    if(depthTarget){const auto d=depthTarget->GetDesc();if(d.Width!=t.Width||d.Height!=t.Height)return true;}   // one size a session
    else {
        D3D12_HEAP_PROPERTIES heapProps{};heapProps.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=t.Width;d.Height=t.Height;
        d.DepthOrArraySize=d.MipLevels=1;d.Format=DXGI_FORMAT_D32_FLOAT;d.SampleDesc.Count=1;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_CLEAR_VALUE clear{};clear.Format=DXGI_FORMAT_D32_FLOAT;clear.DepthStencil.Depth=0;
        if(FAILED(device->CreateCommittedResource(&heapProps,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_DEPTH_WRITE,&clear,IID_PPV_ARGS(&depthTarget)))){depthTarget.Reset();return true;}
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};dsv.Format=DXGI_FORMAT_D32_FLOAT;dsv.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(depthTarget.Get(),&dsv,depthHeap->GetCPUDescriptorHandleForHeapStart());
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC depthView=srv;depthView.Format=depth->format;
    auto depth0=source0;depth0.ptr+=step;
    device->CreateShaderResourceView(depth->depth,&depthView,depth0);
    const auto dsvHandle=depthHeap->GetCPUDescriptorHandleForHeapStart();
    list->ClearDepthStencilView(dsvHandle,D3D12_CLEAR_FLAG_DEPTH,0,0,0,nullptr);
    list->OMSetRenderTargets(1,&rtvHandle,FALSE,&dsvHandle);
    list->SetGraphicsRootSignature(depthRoot.Get());
    list->SetGraphicsRootDescriptorTable(0,sourceGpu);
    const auto tangents=[](const math::Fov& f,float* out){out[0]=std::tan(f.left);out[1]=std::tan(f.right);out[2]=std::tan(f.down);out[3]=std::tan(f.up);};
    const uint32_t across=(uint32_t(t.Width)+TurnGridStep-1)/TurnGridStep,down=(t.Height+TurnGridStep-1)/TurnGridStep;
    uint32_t constants[DepthConstants]{};float values[24]{};
    std::memcpy(values,depth->move.data(),sizeof(float)*12);
    tangents(depth->drawnFov,values+12);tangents(depth->nowFov,values+16);
    values[20]=depth->nearPlane;values[21]=depth->scale[0];values[22]=depth->scale[1];
    std::memcpy(constants,values,sizeof values);constants[24]=across;constants[25]=down;
    const bool carries=depth->reach[1]>depth->reach[0]&&depth->reach[0]>0;
    float carried[16]{};
    std::memcpy(carried,carries?depth->carry.data():depth->move.data(),sizeof(float)*12);
    if(carries){carried[12]=depth->reach[0];carried[13]=depth->reach[1];}
    std::memcpy(constants+28,carried,sizeof carried);
    list->SetGraphicsRoot32BitConstants(1,DepthConstants,constants,0);
    list->SetPipelineState(depthPipeline.Get());
    list->DrawInstanced(6*across*down,1,0,0);
    return true;
}
}
