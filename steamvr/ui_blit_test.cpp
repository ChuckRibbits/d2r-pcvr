#include "ui_blit.h"
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr,const char* what){if(FAILED(hr)){std::fprintf(stderr,"%s: %08lx\n",what,(unsigned long)hr);throw std::runtime_error(what);}}
static float Half(uint16_t x){const int e=(x>>10)&31,m=x&1023;return (x&0x8000?-1.f:1.f)*std::ldexp(e?1.f+m/1024.f:m/1024.f,e?e-15:-14);}
int main(int argc,char**){try {
    const bool blackBackground=argc>1;
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"device");
    framevr::UiBlit blit;if(!blit.Initialize(device.Get(),+[](const char* s){std::puts(s);}))return 1;
    D3D12_RESOURCE_DESC tex{};tex.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;tex.Width=4;tex.Height=2;
    tex.DepthOrArraySize=tex.MipLevels=1;tex.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;tex.SampleDesc.Count=1;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
    ComPtr<ID3D12Resource> source,target,upload,readback;
    Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&tex,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&source)),"source");
    tex.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&tex,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&target)),"target");
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;device->GetCopyableFootprints(&tex,0,1,0,&footprint,nullptr,nullptr,&bytes);
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes;buffer.Height=buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    hp.Type=D3D12_HEAP_TYPE_UPLOAD;Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload)),"upload");
    hp.Type=D3D12_HEAP_TYPE_READBACK;Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)),"readback");
    // Native SDR colors: empty, opaque text, 50% border, 75% icon.
    // At 50% coverage a .5 sRGB border must give .10702 linear, not .25
    // (washed out) or .050876 (decoding premultiplied color directly).
    const uint16_t pixels[8][4]={{0,0,0,0x3c00},{0x3c00,0x3800,0x3400,0},{0x3400,0x3000,0,0x3800},{0x3600,0,0x3200,0x3400},
        {0,0,0,0x4000},{0x3800,0,0,0xbc00},{0x3c00,0x3c00,0x3c00,0},{0,0,0,0}};
    const float expected[8][4]={{0,0,0,0},{1,.21404114f,.05087609f,1},{.10702057f,.02543804f,0,.5f},{.16053085f,0,.03815707f,.75f},
        {0,0,0,0},{.21404114f,0,0,1},{1,1,1,1},{0,0,0,1}};
    void* data=nullptr;D3D12_RANGE empty{0,0};Check(upload->Map(0,&empty,&data),"map upload");
    for(int y=0;y<2;++y)std::memcpy(static_cast<char*>(data)+y*footprint.Footprint.RowPitch,pixels[y*4],32);
    upload->Unmap(0,nullptr);
    D3D12_COMMAND_QUEUE_DESC q{};ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> commands;
    Check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"queue");Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator");
    Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"commands");
    D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprint;
    to.pResource=source.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;commands->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={source.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};
    commands->ResourceBarrier(1,&barrier);
    if(!blit.Render(device.Get(),commands.Get(),source.Get(),target.Get(),blackBackground))throw std::runtime_error("render");
    barrier.Transition={target.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE};commands->ResourceBarrier(1,&barrier);
    from.pResource=target.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;from.SubresourceIndex=0;
    to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=footprint;commands->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    Check(commands->Close(),"close");ID3D12CommandList* lists[]={commands.Get()};queue->ExecuteCommandLists(1,lists);
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fence");Check(queue->Signal(fence.Get(),1),"signal");
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Check(fence->SetEventOnCompletion(1,event),"completion");
    const auto wait=WaitForSingleObject(event,10000);CloseHandle(event);if(wait!=WAIT_OBJECT_0)throw std::runtime_error("GPU timeout");
    D3D12_RANGE range{0,SIZE_T(bytes)};Check(readback->Map(0,&range,&data),"map readback");
    bool correct=true;for(int i=0;i<8;++i){auto* out=reinterpret_cast<uint16_t*>(static_cast<char*>(data)+(i/4)*footprint.Footprint.RowPitch)+(i%4)*4;
        for(int c=0;c<4;++c){const float want=blackBackground&&c==3?1.f:expected[i][c];if(std::abs(Half(out[c])-want)>.0006f){correct=false;std::printf("pixel %d lane %d = %f expected %f\n",i,c,Half(out[c]),want);}}}
    readback->Unmap(0,&empty);if(!correct)throw std::runtime_error("converted pixels differ");
    std::puts(blackBackground?"Native UI GPU conversion passed: opaque black backing, unchanged artwork colors":"Native UI GPU conversion passed: orientation, SDR color, translucent edges, inverse alpha and clamping");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
