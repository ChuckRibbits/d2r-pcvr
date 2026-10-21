#include "ui_blit.h"
#include "dxgi_views.h"
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
SamplerState mapSampler : register(s0);
cbuffer Options : register(b0) {uint blackBackground;uint mapSide;uint mapFlags;float bearing;float wallAlpha;};
// mapFlags (square map only):
//   1 the heading arrow over the map's centre (the hero), turned by bearing
//     (radians, clockwise from map-up);
//   2 the clear background: the map's dark backing and margins see-through,
//     only the walls drawn, at wallAlpha (unassociated alpha, as the wrist quad
//     composes it); a thin gold frame stays;
//   4 the map turned so the heading is up (with 1, the arrow points up);
//  16 (set by Render, square map): FindX ran into `hero` this frame;
//   8 top-down: the game draws its map isometric, the floor grid turned 45
//     degrees and squashed to half height; undo both so rooms are square.
//     The game's map-up becomes the up-left diagonal, so the bearing turns
//     by -45 degrees with it.
// The X: the game's own hero marker at the map's centre. The arrow is drawn
// over it on its own, with a thin dark rim so it reads over walls; no dark
// spot around it.
static const float ArrowSize=.024,RimScale=1.3;   // of the map's side; the rim's dart this much bigger
// a dart: tip ahead, notched tail
float Dart(float2 v,float2 dir,float2 across,float r) {
    float along=dot(v,dir),wide=abs(dot(v,across));
    return step(along,r)*step(-.65*r+.9*wide,along)*step(wide,(r-along)*.42);
}
// 2 inside the arrow, 1 on its rim, 0 elsewhere
float ArrowCoverage(float2 pixel,float side,float angle) {
    float2 v=pixel-side*.5;
    float2 dir=float2(sin(angle),-cos(angle)),across=float2(cos(angle),sin(angle));
    float r=side*ArrowSize;
    return Dart(v,dir,across,r)+Dart(v+dir*r*(RimScale-1)*.3,dir,across,r*RimScale);
}
// The hero's marker: the game's blue X, which is not at the captured map's
// centre (up-right of it on the slanted map) by an amount
// that follows the resolution. FindX, a 1x1 pass before the map's, finds the
// blue pixels nearest the centre and writes their middle (map pixels) and
// count to `hero`; the map is then centred and turned on that point, under the
// arrow, and those blue pixels are painted over.
Texture2D<float4> hero : register(t1);
static const float SearchRadius=.08,MarkerRadius=.02;   // of the map's longer side
bool IsMarker(float4 c) {
    float opacity=1-saturate(c.a);
    if(opacity<.5)return false;
    float3 e=max(c.rgb,0)/opacity;   // sRGB-encoded, unassociated
    return e.b>.7&&e.b>2*e.r&&e.b>1.35*e.g;
}
float4 FindX(float4 position:SV_Position):SV_Target {
    uint w,h;nativeUi.GetDimensions(w,h);
    const float side=max(w,h);
    const int R=int(side*SearchRadius),r=int(side*MarkerRadius)+1;
    const int2 c=int2(uint2(w,h)/2);
    float best=1e9;int2 near=c;
    [loop] for(int y=-R;y<=R;++y) [loop] for(int x=-R;x<=R;++x) {
        int2 q=c+int2(x,y);
        if(any(q<0)||q.x>=int(w)||q.y>=int(h))continue;
        float d=x*x+y*y;
        if(d<best&&IsMarker(nativeUi.Load(int3(q,0)))){best=d;near=q;}
    }
    if(best>1e8)return 0;
    float3 sum=0;
    [loop] for(int y2=-r;y2<=r;++y2) [loop] for(int x2=-r;x2<=r;++x2) {
        int2 q=near+int2(x2,y2);
        if(any(q<0)||q.x>=int(w)||q.y>=int(h))continue;
        if(IsMarker(nativeUi.Load(int3(q,0))))sum+=float3(q+.5,1);
    }
    return float4(sum.xy/max(sum.z,1),sum.z,1);
}
float4 VS(uint id:SV_VertexID):SV_Position {
    return float4(id==2?3:-1,id==1?-3:1,0,1);
}
float4 PS(float4 position:SV_Position):SV_Target {
    float2 pixel=position.xy;
    float2 mapSize=1;
    const bool clearBack=mapSide&&(mapFlags&2);
    // 4: the map turns with you (your heading up, the arrow fixed up); else north-up, the arrow turns
    const bool turning=(mapFlags&4)!=0,topDown=(mapFlags&8)!=0,found=(mapFlags&16)!=0;
    const float heading=topDown?bearing-.78539816:bearing;
    float arrow=0;
    float2 heroAt=0;float markerReach=0;
    if(mapSide&&(mapFlags&1))arrow=ArrowCoverage(position.xy,mapSide,turning?0:heading);
    if(arrow>1.5)return float4(1,.62,.12,1);   // the arrow: a gold dart (linear light)
    if(arrow>.5)return float4(.006,.006,.006,1);   // its dark rim
    if(mapSide) {
        // Fit the entire corner map in a square without stretching, clipping
        // exits or reading it back to the CPU. Empty margins stay dark.
        float bevel=min(1.0,mapSide/128.0);
        float edge=min(min(pixel.x,pixel.y),min(mapSide-pixel.x,mapSide-pixel.y))/bevel;
        // A narrow aged-bronze bevel and inset corner cuts. The entire native
        // map fits inside the frame, including markers at its outer edges.
        float2 corner=min(pixel,float(mapSide)-pixel)/bevel;
        if(edge<1)return float4(.012,.009,.006,1);
        if(edge<3)return float4(.24,.16,.065,1);
        if(edge<5)return float4(.055,.033,.015,1);
        if(edge<7)return float4(.012,.009,.006,1);
        if(max(corner.x,corner.y)<20&&abs(corner.x+corner.y-26)<1.4)
            return float4(.34,.24,.11,1);
        uint w,h;nativeUi.GetDimensions(w,h);
        // 16: FindX ran: centre and turn the map on the hero's marker when it was seen
        const float4 x=found?hero.Load(int3(0,0,0)):0;
        mapSize=float2(w,h);
        // inside upstream's bronze frame (v0.3.18)
        const float inner=mapSide-2*min(20.0,mapSide/8.0);
        float scale=inner/float(max(w,h));
        float2 v=pixel-mapSide*.5;
        if(turning)v=float2(v.x*cos(heading)-v.y*sin(heading),v.x*sin(heading)+v.y*cos(heading));
        // top-down to isometric: (x-y, (x+y)/2), sized so the whole slanted map still fits
        if(topDown)v=float2(v.x-v.y,(v.x+v.y)*.5)*((w+2.*h)/(2.*inner))*scale;
        heroAt=x.z>=3?x.xy:float2(w,h)*.5;
        markerReach=x.z>=3&&(mapFlags&1)?max(w,h)*MarkerRadius*1.6:0;
        pixel=v/scale+heroAt;
        if(any(pixel<0)||pixel.x>=w||pixel.y>=h)return clearBack?0:float4(.006,.006,.006,1);
    }
    float4 c=mapSide?nativeUi.SampleLevel(mapSampler,pixel/mapSize,0):nativeUi.Load(int3(uint2(pixel),0));
    // with the arrow, the game's X itself painted over (the backing shows)
    if(markerReach>0&&distance(pixel,heroAt)<markerReach&&IsMarker(c))return float4(.006,.006,.006,wallAlpha);
    float opacity=1-saturate(c.a);
    // D2R's SDR UI shaders encode sRGB before blending into this float target.
    // XR float layers require linear light. Decode the unassociated color,
    // then premultiply again so antialiased text/borders retain their coverage.
    float3 encoded=max(c.rgb,0)/max(opacity,1e-6);
    float3 linearRgb=lerp(pow((encoded+.055)/1.055,2.4),encoded/12.92,encoded<=.04045);
    // only the walls: unassociated colour, their own coverage at wallAlpha
    // the game's black backing may be drawn as solid pixels: near-black fades out too
    const float lit=smoothstep(.05,.14,max(max(encoded.r,encoded.g),encoded.b));
    if(clearBack)return float4(linearRgb,opacity*lit*wallAlpha);
    float3 rgb=opacity>0?linearRgb*opacity:0;
    if(mapSide)rgb+=.006*(1-opacity);
    // the square map: solid, its black backing too
    return float4(rgb,mapSide?wallAlpha:blackBackground?1:opacity);
}
)";
    ComPtr<ID3DBlob> vs,ps,findPs,errors,signature;
    const auto v=compile(shader,sizeof(shader)-1,"native-ui",nullptr,nullptr,"VS","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vs,&errors);
    const auto p=compile(shader,sizeof(shader)-1,"native-ui",nullptr,nullptr,"PS","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&ps,&errors);
    const auto f=compile(shader,sizeof(shader)-1,"native-ui",nullptr,nullptr,"FindX","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&findPs,&errors);
    if(FAILED(v)||FAILED(p)||FAILED(f)){
        if(log){char b[160];std::snprintf(b,sizeof b,"Frame panels: shader compile failed VS=%08lx PS=%08lx",(unsigned long)v,(unsigned long)p);log(b);if(errors)log(static_cast<const char*>(errors->GetBufferPointer()));}
        return fail();
    }
    D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0};   // t0 the source, t1 the hero marker (FindX)
    D3D12_ROOT_PARAMETER parameters[2]{};auto& parameter=parameters[0];parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameter.DescriptorTable={1,&range};parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[1].Constants={0,0,5};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC desc{};desc.NumParameters=2;desc.pParameters=parameters;
    D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;sampler.MaxLOD=D3D12_FLOAT32_MAX;
    desc.NumStaticSamplers=1;desc.pStaticSamplers=&sampler;
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
    D3D12_DESCRIPTOR_HEAP_DESC heap{};heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;heap.NumDescriptors=2;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    const auto result=device->CreateGraphicsPipelineState(&state,IID_PPV_ARGS(&pipeline));
    if(FAILED(result)) {if(log){char b[128];std::snprintf(b,sizeof b,"Frame panels: pipeline failed %08lx",(unsigned long)result);log(b);}return fail();}
    state.PS={findPs->GetBufferPointer(),findPs->GetBufferSize()};state.RTVFormats[0]=DXGI_FORMAT_R32G32B32A32_FLOAT;
    if(FAILED(device->CreateGraphicsPipelineState(&state,IID_PPV_ARGS(&findPipeline)))){if(log)log("Frame panels: map marker pass unavailable");return fail();}
    if(FAILED(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&sourceHeap))))return fail();
    heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if(FAILED(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&targetHeap))))return fail();
    if(log)log("Frame panels: GPU native UI alpha conversion ready; original artwork retained");
    return true;
}
bool UiBlit::Render(ID3D12Device* device,ID3D12GraphicsCommandList* list,ID3D12Resource* source,ID3D12Resource* target,bool blackBackground,bool squareMap,uint32_t mapFlags,float bearing,float wallAlpha) {
    if(!pipeline||!source||!target)return false;
    const auto s=source->GetDesc(),t=target->GetDesc();
    if(!RenderTargetCompatible(s.Format,DXGI_FORMAT_R16G16B16A16_FLOAT)||
       (squareMap?(t.Width!=t.Height||t.Width<32):(s.Width!=t.Width||s.Height!=t.Height))||
       !RenderTargetCompatible(t.Format,DXGI_FORMAT_R16G16B16A16_FLOAT)||
       s.SampleDesc.Count!=1||t.SampleDesc.Count!=1)return false;
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    device->CreateShaderResourceView(source,&srv,sourceHeap->GetCPUDescriptorHandleForHeapStart());
    const UINT srvStep=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const UINT rtvStep=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto heroSrv=sourceHeap->GetCPUDescriptorHandleForHeapStart();heroSrv.ptr+=srvStep;
    auto heroRtv=targetHeap->GetCPUDescriptorHandleForHeapStart();heroRtv.ptr+=rtvStep;
    // The square map: its hero marker found first (FindX into the 1x1 `hero`), read by the map's pass.
    if(squareMap&&!hero) {
        D3D12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_DEFAULT};
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=d.Height=1;d.DepthOrArraySize=d.MipLevels=1;
        d.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;d.SampleDesc.Count=1;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if(FAILED(device->CreateCommittedResource(&heapProps,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&hero))))hero.Reset();
    }
    const bool findHero=squareMap&&hero&&findPipeline;
    D3D12_SHADER_RESOURCE_VIEW_DESC heroView=srv;heroView.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
    device->CreateShaderResourceView(findHero?hero.Get():nullptr,&heroView,heroSrv);
    list->SetGraphicsRootSignature(root.Get());
    ID3D12DescriptorHeap* heaps[]={sourceHeap.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootDescriptorTable(0,sourceHeap->GetGPUDescriptorHandleForHeapStart());
    auto barrier=[&](D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to) {
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=hero.Get();
        b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=from;b.Transition.StateAfter=to;
        list->ResourceBarrier(1,&b);
    };
    if(findHero) {
        D3D12_RENDER_TARGET_VIEW_DESC heroTarget{};heroTarget.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;heroTarget.ViewDimension=D3D12_RTV_DIMENSION_TEXTURE2D;
        device->CreateRenderTargetView(hero.Get(),&heroTarget,heroRtv);
        list->OMSetRenderTargets(1,&heroRtv,FALSE,nullptr);
        const D3D12_VIEWPORT one{0,0,1,1,0,1};const D3D12_RECT oneRect{0,0,1,1};
        list->RSSetViewports(1,&one);list->RSSetScissorRects(1,&oneRect);
        list->SetPipelineState(findPipeline.Get());
        list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
        barrier(D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        mapFlags|=16;
    }
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};rtvDesc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;rtvDesc.ViewDimension=D3D12_RTV_DIMENSION_TEXTURE2D;
    device->CreateRenderTargetView(target,&rtvDesc,targetHeap->GetCPUDescriptorHandleForHeapStart());
    auto rtv=targetHeap->GetCPUDescriptorHandleForHeapStart();list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    const D3D12_VIEWPORT vp{0,0,float(t.Width),float(t.Height),0,1};const D3D12_RECT rect{0,0,LONG(t.Width),LONG(t.Height)};
    list->RSSetViewports(1,&vp);list->RSSetScissorRects(1,&rect);
    list->SetPipelineState(pipeline.Get());
    list->SetGraphicsRoot32BitConstant(1,blackBackground?1:0,0);
    list->SetGraphicsRoot32BitConstant(1,squareMap?UINT(t.Width):0,1);
    UINT bearingBits=0;std::memcpy(&bearingBits,&bearing,sizeof bearingBits);
    list->SetGraphicsRoot32BitConstant(1,squareMap?mapFlags:0,2);
    list->SetGraphicsRoot32BitConstant(1,bearingBits,3);
    UINT alphaBits=0;std::memcpy(&alphaBits,&wallAlpha,sizeof alphaBits);
    list->SetGraphicsRoot32BitConstant(1,alphaBits,4);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
    if(findHero)barrier(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
    return true;
}
}
