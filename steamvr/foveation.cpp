#include "vrs_compat.h"
#include "foveation.h"
#include "foveation_math.h"
#include <reshade.hpp>
#include <wrl/client.h>
#include <atomic>
#include <array>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace framevr::foveation {
namespace {
using namespace reshade::api;
using Microsoft::WRL::ComPtr;
void (*logger)(const char*)=nullptr;
void Say(const char* s){if(logger)logger(s);}
std::atomic<bool> enabled{true},worldDrawing{false},ready{false};
std::atomic<float> inner{.55f},centerX{.5f},centerY{.5f};
std::atomic<int> eye{0};
std::atomic<uint64_t> maskEpoch{1};
std::atomic<uint32_t> shadedDraws{0},fullDraws{0};
ComPtr<ID3D12Device> nativeDevice;
ComPtr<ID3D12CommandQueue> nativeQueue;
uint32_t screenW=0,screenH=0,tile=0;
struct Pipeline {bool eligible=false;};
std::shared_mutex pipelineLock;
std::unordered_map<uint64_t,Pipeline> pipelines;
struct Mask {
    uint32_t w=0,h=0;int eye=0;float vx=0,vy=0,vw=0,vh=0,cx=0,cy=0,radius=0;
    ComPtr<ID3D12Resource> texture,upload;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commands;
};
std::mutex maskLock;
// Immutable masks and upload objects live until queue completion at shutdown;
// no waits, rewrites or allocation occur for an already cached viewport.
std::vector<Mask> masks;
inline constexpr GUID stateId={0x30889b19,0xcdc7,0x4649,{0xb9,0xb3,0x0d,0xc3,0x62,0x69,0xa2,0x47}};
struct State {
    ComPtr<vrs::CommandList5> native;
    bool queried=false,eligible=false,targets=false,hasMask=false;
    uint32_t w=0,h=0;
    viewport vp{};
    ID3D12Resource* applied=nullptr;
    ID3D12Resource* cached=nullptr;
    uint64_t epoch=0;
};
State* Get(command_list* cl) {
    uint64_t ptr=0;cl->get_private_data(reinterpret_cast<const uint8_t*>(&stateId),&ptr);
    auto* s=reinterpret_cast<State*>(ptr);
    if(!s){s=new State;cl->set_private_data(reinterpret_cast<const uint8_t*>(&stateId),reinterpret_cast<uint64_t>(s));}
    return s;
}
bool Hr(HRESULT hr,const char* what) {
    if(SUCCEEDED(hr))return true;
    char line[160];std::snprintf(line,sizeof line,"Frame foveation: %s failed (0x%08lx); using full quality",what,(unsigned long)hr);Say(line);
    ready=false;return false;
}
ID3D12Resource* GetMask(const State& state) {
    const auto& vp=state.vp;const float radius=inner.load(),cx=centerX.load(),cy=centerY.load();const int e=eye.load();
    std::lock_guard lock(maskLock);
    for(auto& m:masks)if(m.w==state.w&&m.h==state.h&&m.eye==e&&m.vx==vp.x&&m.vy==vp.y&&m.vw==vp.width&&m.vh==vp.height&&m.cx==cx&&m.cy==cy&&m.radius==radius)return m.texture.Get();
    if(masks.size()>=32){Say("Frame foveation: viewport cache limit reached; using full quality until restart");ready=false;return nullptr;}
    Mask m;m.w=state.w;m.h=state.h;m.eye=e;m.vx=vp.x;m.vy=vp.y;m.vw=vp.width;m.vh=vp.height;m.cx=cx;m.cy=cy;m.radius=radius;
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=(m.w+tile-1)/tile;d.Height=(m.h+tile-1)/tile;
    d.DepthOrArraySize=d.MipLevels=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R8_UINT;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    if(!Hr(nativeDevice->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&m.texture)),"create rate image"))return nullptr;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 bytes=0;
    nativeDevice->GetCopyableFootprints(&d,0,1,0,&fp,nullptr,nullptr,&bytes);
    D3D12_RESOURCE_DESC u{};u.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;u.Width=bytes;u.Height=u.DepthOrArraySize=u.MipLevels=u.SampleDesc.Count=1;u.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    heap.Type=D3D12_HEAP_TYPE_UPLOAD;
    if(!Hr(nativeDevice->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&u,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&m.upload)),"create rate upload"))return nullptr;
    uint8_t* data=nullptr;D3D12_RANGE range{};
    if(!Hr(m.upload->Map(0,&range,reinterpret_cast<void**>(&data)),"map rate upload"))return nullptr;
    std::memset(data,0,size_t(bytes));uint32_t coarse=0;
    for(uint32_t y=0;y<d.Height;++y)for(uint32_t x=0;x<d.Width;++x){
        const auto rate=FixedRate(x,y,tile,vp.x,vp.y,vp.width,vp.height,cx,cy,radius);
        data[fp.Offset+y*fp.Footprint.RowPitch+x]=rate;coarse+=rate!=0;
    }
    m.upload->Unmap(0,nullptr);
    if(!Hr(nativeDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&m.allocator)),"create rate allocator")||
       !Hr(nativeDevice->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,m.allocator.Get(),nullptr,IID_PPV_ARGS(&m.commands)),"create rate list"))return nullptr;
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=m.upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=fp;
    dst.pResource=m.texture.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;m.commands->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={m.texture.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,vrs::sourceState};m.commands->ResourceBarrier(1,&barrier);
    if(!Hr(m.commands->Close(),"close rate list"))return nullptr;
    ID3D12CommandList* lists[]={m.commands.Get()};nativeQueue->ExecuteCommandLists(1,lists);
    char line[230];std::snprintf(line,sizeof line,"Frame foveation: eye %d mask %llux%u for %ux%u viewport %.0fx%.0f, center %.3f,%.3f, inner %.2f; %u peripheral 2x2 tiles",e,(unsigned long long)d.Width,d.Height,m.w,m.h,vp.width,vp.height,cx,cy,radius,coarse);Say(line);
    masks.push_back(std::move(m));return masks.back().texture.Get();
}
void Apply(command_list* cl,State& s) {
    const bool want=ready.load(std::memory_order_relaxed)&&enabled.load(std::memory_order_relaxed)&&worldDrawing.load(std::memory_order_relaxed)&&s.eligible&&s.targets&&
        s.vp.width>=screenW*.45f&&s.vp.height>=screenH*.45f&&s.vp.x>=0&&s.vp.y>=0&&s.vp.x+s.vp.width<=s.w+1&&s.vp.y+s.vp.height<=s.h+1&&
        std::abs(s.vp.width/s.vp.height-float(screenW)/screenH)<.04f;
    if(!want&&!s.applied){s.hasMask=false;return;}
    if(!s.queried){
        s.queried=true;auto* native=reinterpret_cast<IUnknown*>(cl->get_native());
        if(FAILED(native->QueryInterface(vrs::commandList5Id,reinterpret_cast<void**>(s.native.GetAddressOf())))){
            Say("Frame foveation: CommandList5 unavailable; using full quality");ready=false;return;
        }
    }
    if(!s.native)return;
    ID3D12Resource* image=nullptr;
    if(want){const auto current=maskEpoch.load();if(s.epoch!=current){s.cached=GetMask(s);s.epoch=current;}image=s.cached;}
    s.hasMask=image!=nullptr;
    if(s.applied==image)return;
    const vrs::Combiner combines[]={vrs::Combiner::passthrough,image?vrs::Combiner::overrideRate:vrs::Combiner::passthrough};
    s.native->RSSetShadingRateImage(image);s.native->RSSetShadingRate(vrs::Rate::full,combines);s.applied=image;
}
void PipelineCreated(device*,pipeline_layout,uint32_t count,const pipeline_subobject* parts,pipeline pipe) {
    bool hasPixel=false,depth=false,writes=false,opaque=true,single=true;
    for(uint32_t i=0;i<count;++i){
        if(!parts[i].count||!parts[i].data)continue;
        switch(parts[i].type){
        case pipeline_subobject_type::pixel_shader:{const auto* p=static_cast<const shader_desc*>(parts[i].data);hasPixel=p->code&&p->code_size;break;}
        case pipeline_subobject_type::depth_stencil_state:{const auto* d=static_cast<const depth_stencil_desc*>(parts[i].data);depth=d->depth_enable;writes=d->depth_write_mask;break;}
        case pipeline_subobject_type::blend_state:{const auto* b=static_cast<const blend_desc*>(parts[i].data);opaque=!b->alpha_to_coverage_enable;for(int k=0;k<8;++k)opaque&=!b->blend_enable[k]&&!b->logic_op_enable[k];break;}
        case pipeline_subobject_type::sample_count:single=*static_cast<const uint32_t*>(parts[i].data)==1;break;
        default:break;
        }
    }
    std::unique_lock lock(pipelineLock);pipelines[pipe.handle]={hasPixel&&depth&&writes&&opaque&&single};
}
void PipelineDestroyed(device*,pipeline p){std::unique_lock lock(pipelineLock);pipelines.erase(p.handle);}
void BindPipeline(command_list* cl,pipeline_stage stages,pipeline p){
    if(uint32_t(stages&pipeline_stage::all_graphics)==0)return;
    auto& s=*Get(cl);{std::shared_lock lock(pipelineLock);const auto it=pipelines.find(p.handle);s.eligible=it!=pipelines.end()&&it->second.eligible;}Apply(cl,s);
}
void BindTargets(command_list* cl,uint32_t count,const resource_view* rtvs,resource_view dsv){
    auto& s=*Get(cl);const auto oldW=s.w,oldH=s.h;s.targets=false;s.w=s.h=0;
    if(ready&&count>0&&count<=8&&rtvs&&rtvs[0].handle&&dsv.handle){
        auto* dev=cl->get_device();const auto color=dev->get_resource_desc(dev->get_resource_from_view(rtvs[0]));
        const auto depth=dev->get_resource_desc(dev->get_resource_from_view(dsv));
        s.w=color.texture.width;s.h=color.texture.height;
        s.targets=color.type==resource_type::texture_2d&&color.texture.samples==1&&depth.texture.width==s.w&&depth.texture.height==s.h&&
            s.w>=screenW*.45f&&s.h>=screenH*.45f&&std::abs(float(s.w)/s.h-float(screenW)/screenH)<.04f;
    }
    if(s.w!=oldW||s.h!=oldH)s.epoch=0;
    Apply(cl,s);
}
bool BeginPass(command_list* cl,uint32_t count,const render_pass_render_target_desc* rts,const render_pass_depth_stencil_desc* ds,render_pass_flags){
    std::array<resource_view,8> rtvs{};for(uint32_t i=0;i<std::min(count,8u);++i)rtvs[i]=rts[i].view;
    BindTargets(cl,std::min(count,8u),rtvs.data(),ds?ds->view:resource_view{});return false;
}
bool EndPass(command_list* cl){auto& s=*Get(cl);s.targets=false;Apply(cl,s);return false;}
void Viewports(command_list* cl,uint32_t first,uint32_t count,const viewport* vps){
    auto& s=*Get(cl);const auto old=s.vp;if(first==0&&count==1)s.vp=vps[0];else s.vp={};
    if(std::memcmp(&old,&s.vp,sizeof old)!=0)s.epoch=0;Apply(cl,s);
}
bool Draw(command_list* cl,uint32_t,uint32_t,uint32_t,uint32_t){auto& s=*Get(cl);Apply(cl,s);if(s.hasMask)++shadedDraws;else ++fullDraws;return false;}
bool DrawIndexed(command_list* cl,uint32_t,uint32_t,uint32_t,int32_t,uint32_t){return Draw(cl,0,0,0,0);}
void Reset(command_list* cl){auto& s=*Get(cl);s.eligible=s.targets=s.hasMask=false;s.w=s.h=0;s.vp={};s.applied=s.cached=nullptr;s.epoch=0;}
void Destroy(command_list* cl){delete Get(cl);cl->set_private_data(reinterpret_cast<const uint8_t*>(&stateId),0);}
void Effects(effect_runtime*,command_list* cl,resource_view,resource_view){Suspend(cl);}
}
void Register(void (*log)(const char*)) {
    logger=log;
    reshade::register_event<reshade::addon_event::init_pipeline>(&PipelineCreated);
    reshade::register_event<reshade::addon_event::destroy_pipeline>(&PipelineDestroyed);
    reshade::register_event<reshade::addon_event::bind_pipeline>(&BindPipeline);
    reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(&BindTargets);
    reshade::register_event<reshade::addon_event::begin_render_pass>(&BeginPass);
    reshade::register_event<reshade::addon_event::end_render_pass>(&EndPass);
    reshade::register_event<reshade::addon_event::bind_viewports>(&Viewports);
    reshade::register_event<reshade::addon_event::draw>(&Draw);
    reshade::register_event<reshade::addon_event::draw_indexed>(&DrawIndexed);
    reshade::register_event<reshade::addon_event::reset_command_list>(&Reset);
    reshade::register_event<reshade::addon_event::destroy_command_list>(&Destroy);
    reshade::register_event<reshade::addon_event::reshade_begin_effects>(&Effects);
}
void Configure(bool on,float radius){enabled=on;const float value=std::isfinite(radius)?std::clamp(radius,.35f,.85f):.55f;if(inner.exchange(value)!=value)++maskEpoch;}
void Initialize(ID3D12Device* dev,ID3D12CommandQueue* q,uint32_t w,uint32_t h){
    nativeDevice=dev;nativeQueue=q;screenW=w;screenH=h;vrs::Features f{};
    ready=SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6,&f,sizeof f))&&f.tier>=2&&(f.tile==8||f.tile==16||f.tile==32);
    tile=f.tile;char line[160];std::snprintf(line,sizeof line,"Frame foveation: fixed VRS %s; tier %u tile %u; 1x1 center / 2x2 opaque-world periphery",ready?"available":"unavailable",f.tier,tile);Say(line);
}
void BeginEye(int e,float cx,float cy){eye=e;centerX=cx;centerY=cy;++maskEpoch;worldDrawing=true;}
void EndWorld(){worldDrawing=false;}
void Suspend(command_list* cl){auto& s=*Get(cl);s.targets=false;Apply(cl,s);}
Stats TakeStats(){return {shadedDraws.exchange(0),fullDraws.exchange(0)};}
void Shutdown(){ready=false;worldDrawing=false;std::lock_guard lock(maskLock);masks.clear();nativeQueue.Reset();nativeDevice.Reset();}
}
