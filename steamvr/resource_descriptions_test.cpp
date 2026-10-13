#include "resource_descriptions.h"
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
using namespace framevr;
using namespace reshade::api;
namespace {
resource_desc Texture(uint32_t side) {
    return resource_desc(side,side,1,1,format::r8g8b8a8_unorm,1,memory_heap::default_,resource_usage::render_target);
}
void Correctness() {
    auto* dev=reinterpret_cast<device*>(uintptr_t(1));
    auto* other=reinterpret_cast<device*>(uintptr_t(2));
    resources::Descriptions cache;
    unsigned reads=0;uint32_t side=200;
    auto read=[&](resource){++reads;return Texture(side);};
    assert(cache.Get(dev,{0},read).type==resource_type::unknown&&reads==0);
    for(int i=0;i<1000;++i)assert(cache.Get(dev,{0x1000},read).texture.width==200);
    assert(reads==1); // Repeated target bindings never requery immutable metadata.
    side=400;
    resources::Destroyed(dev,{0x1000});
    resources::Created(dev,Texture(side),nullptr,resource_usage::render_target,{0x1000});
    assert(cache.Get(dev,{0x1000},read).texture.width==400&&reads==2);
    assert(cache.Get(other,{0x1000},read).texture.width==400&&reads==3);
    assert(cache.Get(dev,{0x1000},read).texture.width==400&&reads==4);
    resources::DeviceDestroyed(dev);
    assert(cache.Get(dev,{0x1000},read).texture.width==400&&reads==5);

    // An RTV/DSV handle can be copied/rebound without init_resource_view.
    // Resolve its current RESOURCE first, then look up that resource's metadata.
    resource target{0x2000};auto resolve=[&](resource_view){return target;};
    auto view=[&]{return cache.Get(dev,resolve({0x80}),[&](resource r){return Texture(r.handle==0x2000?600:800);});};
    assert(view().texture.width==600);target={0x3000};assert(view().texture.width==800);
    target={0x2000};assert(view().texture.width==600);
    // A bounded cache collision must miss rather than returning another size.
    GenerationCache<resource_desc,2> tiny;resource_desc result;
    tiny.Put(0,1,Texture(10));tiny.Put(2,1,Texture(20));
    assert(!tiny.Get(0,1,result)&&tiny.Get(2,1,result)&&result.texture.width==20);

    std::atomic<unsigned> calls=0;std::vector<std::thread> workers;
    for(unsigned t=0;t<8;++t)workers.emplace_back([&,t]{
        resources::Descriptions local;
        for(unsigned i=0;i<10000;++i) {
            const auto desc=local.Get(dev,{0x4000},[&](resource){++calls;return Texture(100+t);});
            assert(desc.texture.width==100+t);
        }
    });
    for(auto& worker:workers)worker.join();
    assert(calls==8); // Independent workers own copied metadata, no shared map lock.
    std::puts("Resource description reuse, lifetime/device invalidation, descriptor rebinding and worker isolation passed");
}
void Benchmark() {
    using Microsoft::WRL::ComPtr;
    ComPtr<ID3D12Device> native;
    assert(SUCCEEDED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&native))));
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC texture{};texture.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture.Width=texture.Height=64;texture.DepthOrArraySize=texture.MipLevels=texture.SampleDesc.Count=1;
    texture.Format=DXGI_FORMAT_R8G8B8A8_UNORM;texture.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    ComPtr<ID3D12Resource> image;
    assert(SUCCEEDED(native->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&texture,
        D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&image))));
    // Query the same native metadata used by ReShade's D3D12 get_resource_desc.
    // Setup is excluded; this is an isolated callback-path measurement, not FPS.
    auto read=[&](resource){
        D3D12_HEAP_PROPERTIES props{};D3D12_HEAP_FLAGS flags{};
        const auto hr=image->GetHeapProperties(&props,&flags);const auto d=image->GetDesc();
        assert(SUCCEEDED(hr)&&props.Type==D3D12_HEAP_TYPE_DEFAULT&&d.Width==64);
        return Texture(uint32_t(d.Width));
    };
    auto* dev=reinterpret_cast<device*>(native.Get());
    resource target{reinterpret_cast<uintptr_t>(image.Get())};
    for(unsigned threads:{1u,8u})for(bool cached:{false,true}) {
        constexpr unsigned iterations=100000;
        std::atomic<unsigned> ready=0;std::atomic<bool> go=false;
        std::vector<std::thread> workers;
        for(unsigned t=0;t<threads;++t)workers.emplace_back([&]{
            resources::Descriptions local;++ready;
            while(!go.load(std::memory_order_acquire))SwitchToThread();
            for(unsigned i=0;i<iterations;++i) {
                auto desc=cached?local.Get(dev,target,read):read(target);
                assert(desc.texture.width==64);
            }
        });
        while(ready!=threads)SwitchToThread();
        const auto start=hookmetrics::Nanos();go.store(true,std::memory_order_release);
        for(auto& worker:workers)worker.join();
        std::printf("Isolated D3D12 description benchmark: threads=%u cached=%d wall ns/query=%.2f (not game FPS)\n",
            threads,cached,double(hookmetrics::Nanos()-start)/(threads*iterations));
    }
}
}
int main(int argc,char** argv) {
    Correctness();
    if(argc==2&&std::strcmp(argv[1],"--benchmark")==0)Benchmark();
}
