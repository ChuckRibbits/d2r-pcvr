#include "turn_warp.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
using framevr::math::Fov;
using framevr::math::Quat;
static void Check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
static void Check(HRESULT hr,const char* what,int){if(FAILED(hr)){std::fprintf(stderr,"%s: %08lx\n",what,(unsigned long)hr);throw std::runtime_error(what);}}
static bool Near(float a,float b,float e=1e-5f){return std::fabs(a-b)<=e;}
// where this frame's (u, v) is in the drawn image
static void Apply(const std::array<float,9>& h,float u,float v,float& x,float& y,float& w) {
    w=h[6]*u+h[7]*v+h[8];x=(h[0]*u+h[1]*v+h[2])/w;y=(h[3]*u+h[4]*v+h[5])/w;
}
static Quat Pitch(float a){return {std::sin(a*.5f),0,0,std::cos(a*.5f)};}

static void Math() {
    const Fov steam{-1.03239f,1.03239f,-1.04476f,1.04476f};   // the owner's headset, symmetric
    const Fov offset{-.95f,.80f,-.90f,.85f};                      // a runtime's own, off-centre
    const Quat level{0,0,0,1},tilted=framevr::math::Multiply(framevr::math::Yaw(.7f),Pitch(-.3f));
    for(const Fov& fov:{steam,offset})for(const Quat& head:{level,tilted}) {
        const auto same=framevr::TurnHomography(head,fov,head,fov);
        for(float u:{0.f,.25f,.5f,1.f})for(float v:{0.f,.4f,1.f}) {
            float x,y,w;Apply(same,u,v,x,y,w);
            Check(Near(x,u)&&Near(y,v)&&w>0,"the same head and FOV leave every pixel where it is");
        }
    }
    const float t=std::tan(steam.right),turn=.05f;   // about 3 degrees in a frame
    float x,y,w;
    // the head turned left: this frame's middle was left of the drawn image's
    Apply(framevr::TurnHomography(level,steam,framevr::math::Yaw(turn),steam),.5f,.5f,x,y,w);
    Check(Near(x,.5f-std::tan(turn)/(2*t))&&Near(y,.5f)&&w>0,"a turn to the left reads left of the drawn middle");
    Apply(framevr::TurnHomography(level,steam,framevr::math::Yaw(-turn),steam),.5f,.5f,x,y,w);
    Check(Near(x,.5f+std::tan(turn)/(2*t))&&Near(y,.5f),"a turn to the right reads right of it");
    // looking up: higher in the drawn image (v down)
    Apply(framevr::TurnHomography(level,steam,Pitch(turn),steam),.5f,.5f,x,y,w);
    Check(Near(x,.5f)&&Near(y,.5f-std::tan(turn)/(2*std::tan(steam.up))),"a look up reads above the drawn middle");
    // the turn is relative: the same 3 degrees from any head
    const auto from=framevr::math::Multiply(framevr::math::Yaw(1.1f),Pitch(.2f));
    Apply(framevr::TurnHomography(from,steam,framevr::math::Multiply(from,framevr::math::Yaw(turn)),steam),.5f,.5f,x,y,w);
    Check(Near(x,.5f-std::tan(turn)/(2*t),1e-4f)&&Near(y,.5f,1e-4f),"only the turn since the drawing counts");
    // a narrower FOV now reads nearer the drawn middle
    const Fov narrow{-.8f,.8f,-.8f,.8f};
    Apply(framevr::TurnHomography(level,steam,level,narrow),1.f,0.f,x,y,w);
    Check(Near(x,.5f+std::tan(.8f)/(2*t))&&Near(y,.5f-std::tan(.8f)/(2*std::tan(steam.up))),"each image's own FOV");
    // the right edge of an off-centre FOV
    Apply(framevr::TurnHomography(level,offset,level,steam),1.f,.5f,x,y,w);
    const float L=std::tan(offset.left),R=std::tan(offset.right);
    Check(Near(x,(t-L)/(R-L)),"an off-centre drawn FOV");
    std::puts("Turn homography passed: identity, yaw, pitch, relative turns, FOVs");
}

// The pass on the GPU: an 8x8 picture, the target typed or typeless as the runtime's may be.
static void Gpu(bool typeless) {
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"device",0);
    framevr::TurnWarp warp;
    Check(warp.Initialize(device.Get(),DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,+[](const char* s){std::puts(s);}),"pipeline");
    constexpr UINT side=8;
    D3D12_RESOURCE_DESC tex{};tex.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;tex.Width=tex.Height=side;
    tex.DepthOrArraySize=tex.MipLevels=1;tex.Format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;tex.SampleDesc.Count=1;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;
    device->GetCopyableFootprints(&tex,0,1,0,&footprint,nullptr,nullptr,&bytes);
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
    ComPtr<ID3D12Resource> source,target,upload,readback;
    Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&tex,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&source)),"source",0);
    tex.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;if(typeless)tex.Format=DXGI_FORMAT_R8G8B8A8_TYPELESS;
    Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&tex,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&target)),"target",0);
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes;buffer.Height=buffer.DepthOrArraySize=buffer.MipLevels=1;
    buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    hp.Type=D3D12_HEAP_TYPE_UPLOAD;Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload)),"upload",0);
    hp.Type=D3D12_HEAP_TYPE_READBACK;Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)),"readback",0);
    // every pixel its own: x and y in red and green, a fixed blue, opaque
    const auto texel=[](UINT x,UINT y){return uint32_t(20+x*29)|uint32_t(20+y*29)<<8|0x80u<<16|0xffu<<24;};
    void* data=nullptr;D3D12_RANGE none{0,0};Check(upload->Map(0,&none,&data),"map upload",0);
    for(UINT y=0;y<side;++y)for(UINT x=0;x<side;++x)reinterpret_cast<uint32_t*>(static_cast<char*>(data)+y*footprint.Footprint.RowPitch)[x]=texel(x,y);
    upload->Unmap(0,nullptr);
    D3D12_COMMAND_QUEUE_DESC q{};ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    Check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"queue",0);
    Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator",0);
    Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"list",0);
    D3D12_TEXTURE_COPY_LOCATION from{},to{};
    from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprint;
    to.pResource=source.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={source.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};
    list->ResourceBarrier(1,&barrier);Check(list->Close(),"close upload",0);
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fence",0);
    uint64_t fenceValue=0;
    const auto run=[&](){
        ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);
        Check(queue->Signal(fence.Get(),++fenceValue),"signal",0);
        HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Check(fence->SetEventOnCompletion(fenceValue,event),"completion",0);
        const auto waited=WaitForSingleObject(event,10000);CloseHandle(event);
        if(waited!=WAIT_OBJECT_0)throw std::runtime_error("GPU timeout");
    };
    run();
    // what each case should give: a source pixel (x, y), or black when behind
    struct Case {const char* what;std::array<float,9> h;int dx,dy;bool behind;};
    const Case cases[]={
        {"the same head copies every pixel",{1,0,0, 0,1,0, 0,0,1},0,0,false},
        {"a turn reads two pixels to the right, the edge drawn out",{1,0,2.f/side, 0,1,0, 0,0,1},2,0,false},
        {"homogeneous: a scale of the whole matrix changes nothing",{3,0,0, 0,3,-3.f/side, 0,0,3},0,-1,false},
        {"slot 1 and a look down: one pixel lower",{1,0,0, 0,1,1.f/side, 0,0,1},0,1,false},
        {"behind the drawn eye stays black",{1,0,0, 0,1,0, 0,0,-1},0,0,true},
    };
    for(const auto& c:cases) {
        const int slot=int(&c-cases)&1;
        Check(allocator->Reset(),"reset allocator",0);Check(list->Reset(allocator.Get(),nullptr),"reset list",0);
        Check(warp.Render(device.Get(),list.Get(),source.Get(),target.Get(),c.h,slot),"render");
        barrier.Transition={target.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE};
        list->ResourceBarrier(1,&barrier);
        from.pResource=target.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;from.SubresourceIndex=0;
        to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=footprint;
        list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);list->ResourceBarrier(1,&barrier);
        Check(list->Close(),"close",0);run();
        D3D12_RANGE range{0,SIZE_T(bytes)};Check(readback->Map(0,&range,&data),"map readback",0);
        bool same=true;
        for(UINT y=0;y<side;++y)for(UINT x=0;x<side;++x) {
            const uint32_t got=reinterpret_cast<const uint32_t*>(static_cast<const char*>(data)+y*footprint.Footprint.RowPitch)[x];
            const UINT sx=UINT(std::clamp(int(x)+c.dx,0,int(side)-1)),sy=UINT(std::clamp(int(y)+c.dy,0,int(side)-1));
            const uint32_t want=c.behind?0xff000000u:texel(sx,sy);
            // sRGB decoded, filtered at the texel's centre and encoded again: a step at most
            for(int lane=0;lane<4;++lane)if(std::abs(int((got>>lane*8)&255)-int((want>>lane*8)&255))>1) {
                same=false;std::printf("%s: pixel %u,%u lane %d = %u, expected %u\n",c.what,x,y,lane,(got>>lane*8)&255,(want>>lane*8)&255);
            }
        }
        readback->Unmap(0,&none);
        Check(same,c.what);
    }
    std::printf("Turn warp GPU pass passed (%s target): copy, turn, edge, scale, slots, behind\n",typeless?"typeless":"typed");
}

int main(int argc,char** argv) {try {
    const bool typeless=argc>1&&!std::strcmp(argv[1],"--typeless");
    Math();Gpu(typeless);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
