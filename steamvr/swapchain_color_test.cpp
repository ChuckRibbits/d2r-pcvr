// Simulate a compositor reading a copied SDR image through its swapchain format.
// A runtime exposing UNORM before SRGB previously made the midtones too bright.
#include "swapchain_format.h"
#include <windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr,const char* what) {
    if(FAILED(hr)){std::fprintf(stderr,"%s: %08lx\n",what,(unsigned long)hr);throw std::runtime_error(what);}
}
static void Require(bool condition,const char* what){if(!condition)throw std::runtime_error(what);}
static void ReadCopiedImage(DXGI_FORMAT sourceFormat,std::span<const int64_t> formats) {
    const auto selected=framevr::CopiedSwapchainFormat(sourceFormat,formats);
    Require(selected!=DXGI_FORMAT_UNKNOWN,"no selected color format");
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"device");
    D3D12_RESOURCE_DESC texture{};texture.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture.Width=5;texture.Height=texture.DepthOrArraySize=texture.MipLevels=texture.SampleDesc.Count=1;texture.Format=sourceFormat;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    ComPtr<ID3D12Resource> source,eye,upload,output,readback;
    Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&texture,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&source)),"game image");
    texture.Format=sourceFormat==DXGI_FORMAT_B8G8R8A8_UNORM?DXGI_FORMAT_B8G8R8A8_TYPELESS:DXGI_FORMAT_R8G8B8A8_TYPELESS;
    Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&texture,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&eye)),"runtime image");
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;const auto sourceDesc=source->GetDesc();
    device->GetCopyableFootprints(&sourceDesc,0,1,0,&footprint,nullptr,nullptr,&bytes);
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes;
    buffer.Height=buffer.DepthOrArraySize=buffer.MipLevels=buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    heap.Type=D3D12_HEAP_TYPE_UPLOAD;
    Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload)),"upload");
    buffer.Width=5*4*sizeof(float);heap.Type=D3D12_HEAP_TYPE_READBACK;
    Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)),"readback");
    buffer.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&output)),"linear output");
    constexpr uint8_t ramp[]={0,64,128,192,255};void* data=nullptr;D3D12_RANGE empty{};
    Check(upload->Map(0,&empty,&data),"map upload");std::memset(data,0,size_t(bytes));
    for(int i=0;i<5;++i){auto* pixel=static_cast<uint8_t*>(data)+footprint.Offset+i*4;pixel[0]=pixel[1]=pixel[2]=ramp[i];pixel[3]=64;}
    upload->Unmap(0,nullptr);
    const char* shader="Texture2D<float4> eye:register(t0); RWStructuredBuffer<float4> result:register(u0); [numthreads(5,1,1)] void main(uint3 id:SV_DispatchThreadID){result[id.x]=eye.Load(int3(id.x,0,0));}";
    ComPtr<ID3DBlob> compiled,errors,signature;
    Check(D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,"main","cs_5_0",0,0,&compiled,&errors),"compile");
    D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};D3D12_ROOT_PARAMETER parameters[2]{};
    parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[0].DescriptorTable={1,&range};
    parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;parameters[1].Descriptor={0,0};
    D3D12_ROOT_SIGNATURE_DESC rootDesc{};rootDesc.NumParameters=2;rootDesc.pParameters=parameters;
    Check(D3D12SerializeRootSignature(&rootDesc,D3D_ROOT_SIGNATURE_VERSION_1,&signature,&errors),"serialize root");
    ComPtr<ID3D12RootSignature> root;Check(device->CreateRootSignature(0,signature->GetBufferPointer(),signature->GetBufferSize(),IID_PPV_ARGS(&root)),"root");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc{};pipelineDesc.pRootSignature=root.Get();pipelineDesc.CS={compiled->GetBufferPointer(),compiled->GetBufferSize()};
    ComPtr<ID3D12PipelineState> pipeline;Check(device->CreateComputePipelineState(&pipelineDesc,IID_PPV_ARGS(&pipeline)),"pipeline");
    D3D12_DESCRIPTOR_HEAP_DESC descriptors{};descriptors.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;descriptors.NumDescriptors=1;descriptors.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> srvHeap;Check(device->CreateDescriptorHeap(&descriptors,IID_PPV_ARGS(&srvHeap)),"SRV heap");
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=selected;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    device->CreateShaderResourceView(eye.Get(),&srv,srvHeap->GetCPUDescriptorHandleForHeapStart());
    D3D12_COMMAND_QUEUE_DESC queueDesc{};ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> commands;
    Check(device->CreateCommandQueue(&queueDesc,IID_PPV_ARGS(&queue)),"queue");Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator");
    Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"commands");
    D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprint;to.pResource=source.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    commands->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={source.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE};commands->ResourceBarrier(1,&barrier);
    commands->CopyResource(eye.Get(),source.Get());
    barrier.Transition={eye.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};commands->ResourceBarrier(1,&barrier);
    commands->SetComputeRootSignature(root.Get());commands->SetPipelineState(pipeline.Get());ID3D12DescriptorHeap* heaps[]={srvHeap.Get()};commands->SetDescriptorHeaps(1,heaps);
    commands->SetComputeRootDescriptorTable(0,srvHeap->GetGPUDescriptorHandleForHeapStart());commands->SetComputeRootUnorderedAccessView(1,output->GetGPUVirtualAddress());commands->Dispatch(1,1,1);
    barrier.Transition={output.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};commands->ResourceBarrier(1,&barrier);commands->CopyResource(readback.Get(),output.Get());
    Check(commands->Close(),"close");ID3D12CommandList* lists[]={commands.Get()};queue->ExecuteCommandLists(1,lists);
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fence");Check(queue->Signal(fence.Get(),1),"signal");
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Check(fence->SetEventOnCompletion(1,event),"completion");const auto wait=WaitForSingleObject(event,10000);CloseHandle(event);Require(wait==WAIT_OBJECT_0,"GPU timeout");
    D3D12_RANGE readRange{0,5*4*sizeof(float)};Check(readback->Map(0,&readRange,&data),"map result");
    const auto* values=static_cast<const float*>(data);bool correct=true;
    for(int i=0;i<5;++i){const float encoded=ramp[i]/255.f;const float linear=encoded<=.04045f?encoded/12.92f:std::pow((encoded+.055f)/1.055f,2.4f);
        for(int c=0;c<4;++c){const float want=c==3?64/255.f:linear;if(std::abs(values[i*4+c]-want)>.002f){correct=false;std::fprintf(stderr,"format %d pixel %d channel %d: %f expected %f\n",int(selected),i,c,values[i*4+c],want);}}}
    readback->Unmap(0,&empty);Require(correct,"copied sRGB image was interpreted with the wrong transfer function");
}
int main(){try {
    const std::array<int64_t,3> both{DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_R16G16B16A16_FLOAT};
    const std::array<int64_t,1> srgb{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB},linear{DXGI_FORMAT_R8G8B8A8_UNORM};
    const std::array<int64_t,2> bgra{DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB};
    ReadCopiedImage(DXGI_FORMAT_R8G8B8A8_UNORM,both);ReadCopiedImage(DXGI_FORMAT_R8G8B8A8_UNORM,srgb);ReadCopiedImage(DXGI_FORMAT_B8G8R8A8_UNORM,bgra);
    Require(framevr::CopiedSwapchainFormat(DXGI_FORMAT_R8G8B8A8_UNORM,linear)==DXGI_FORMAT_UNKNOWN,"linear-only runtime must not silently misinterpret SDR bytes");
    Require(framevr::CopiedSwapchainFormat(DXGI_FORMAT_R16G16B16A16_FLOAT,both)==DXGI_FORMAT_R16G16B16A16_FLOAT,"float source format must remain linear");
    Require(framevr::CopiedSwapchainFormat(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,both)==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,"already encoded format must remain sRGB");
    std::puts("Copied SDR GPU color test passed: RGBA/BGRA midtones decoded once, alpha preserved, float unchanged");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
