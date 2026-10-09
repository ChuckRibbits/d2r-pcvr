#pragma once
#include <d3d12.h>

// The installed MinGW headers stop at CommandList2. These extension slots and
// values follow Microsoft's MIT-licensed DirectX-Headers d3d12.h. The intervening
// methods are never called here; retain their order and ABI before the two VRS
// methods. QueryInterface checks that the runtime actually implements CL5.
namespace framevr::vrs {
enum class Rate : UINT { full=0, halfX=4, halfY=1, quarter=5 };
enum class Combiner : UINT { passthrough=0, overrideRate=1 };
inline const D3D12_RESOURCE_STATES sourceState=D3D12_RESOURCE_STATES(0x1000000);
inline constexpr GUID commandList5Id={0x55050859,0x4024,0x474c,{0x87,0xf5,0x64,0x72,0xea,0xee,0x44,0xea}};
struct CommandList5 : ID3D12GraphicsCommandList2 {
    virtual void STDMETHODCALLTYPE SetProtectedResourceSession(void*)=0;
    virtual void STDMETHODCALLTYPE BeginRenderPass(UINT,const void*,const void*,UINT)=0;
    virtual void STDMETHODCALLTYPE EndRenderPass()=0;
    virtual void STDMETHODCALLTYPE InitializeMetaCommand(void*,const void*,SIZE_T)=0;
    virtual void STDMETHODCALLTYPE ExecuteMetaCommand(void*,const void*,SIZE_T)=0;
    virtual void STDMETHODCALLTYPE BuildRaytracingAccelerationStructure(const void*,UINT,const void*)=0;
    virtual void STDMETHODCALLTYPE EmitRaytracingAccelerationStructurePostbuildInfo(const void*,UINT,const D3D12_GPU_VIRTUAL_ADDRESS*)=0;
    virtual void STDMETHODCALLTYPE CopyRaytracingAccelerationStructure(D3D12_GPU_VIRTUAL_ADDRESS,D3D12_GPU_VIRTUAL_ADDRESS,UINT)=0;
    virtual void STDMETHODCALLTYPE SetPipelineState1(void*)=0;
    virtual void STDMETHODCALLTYPE DispatchRays(const void*)=0;
    virtual void STDMETHODCALLTYPE RSSetShadingRate(Rate,const Combiner*)=0;
    virtual void STDMETHODCALLTYPE RSSetShadingRateImage(ID3D12Resource*)=0;
};
struct Features {
    BOOL additionalRates,perPrimitiveViewportIndex;
    UINT tier,tile;
    BOOL backgroundProcessing;
};
static_assert(sizeof(Features)==20);
}
