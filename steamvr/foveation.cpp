#include "vrs_compat.h"
#include "foveation.h"
#include "profile.h"
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
std::atomic<bool> enabled{true},worldDrawing{false},ready{false},gazeDriven{false};
std::atomic<float> inner{.55f},centerX{.5f},centerY{.5f};
std::atomic<float> gazeInner{.35f};
std::atomic<int> eye{0};
std::atomic<uint64_t> maskEpoch{1};
std::atomic<uint64_t> stateGeneration{1},pipelineGeneration{1};
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
struct GazeBank {
    uint32_t w=0,h=0;float vx=0,vy=0,vw=0,vh=0,radius=0;
    std::array<ComPtr<ID3D12Resource>,GazeMaskCount> textures;
    ComPtr<ID3D12Resource> upload;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commands;
};
// Shared between eyes, independent of the actual gaze path. Upload the complete
// grid once per viewport/radius; never rewrite a resource still in GPU use.
std::vector<GazeBank> gazeBanks;
inline constexpr GUID stateId={0x30889b19,0xcdc7,0x4649,{0xb9,0xb3,0x0d,0xc3,0x62,0x69,0xa2,0x47}};
struct State {
    ComPtr<vrs::CommandList5> native;
    bool queried=false,eligible=false,targets=false,hasMask=false;
    uint32_t w=0,h=0;
    viewport vp{};
    ID3D12Resource* applied=nullptr;
    ID3D12Resource* cached=nullptr;
    uint64_t epoch=0;
    uint64_t appliedEpoch=0,pipeline=0,pipelineEpoch=0;
    bool dirty=true;
};
// ReShade's private-data lookup synchronizes internally. A command list records
// on one thread at a time, so retain its state across consecutive callbacks.
// Destruction invalidates every thread's cache before a recycled pointer is used.
struct StateCache {command_list* cl=nullptr;State* state=nullptr;uint64_t generation=0;};
thread_local StateCache stateCache;
State* Get(command_list* cl) {
    const auto generation=stateGeneration.load(std::memory_order_relaxed);
    if(stateCache.cl==cl&&stateCache.generation==generation)return stateCache.state;
    uint64_t ptr=0;cl->get_private_data(reinterpret_cast<const uint8_t*>(&stateId),&ptr);
    auto* s=reinterpret_cast<State*>(ptr);
    if(!s){s=new State;cl->set_private_data(reinterpret_cast<const uint8_t*>(&stateId),reinterpret_cast<uint64_t>(s));}
    stateCache={cl,s,generation};
    return s;
}
bool Hr(HRESULT hr,const char* what) {
    if(SUCCEEDED(hr))return true;
    char line[160];std::snprintf(line,sizeof line,"Frame foveation: %s failed (0x%08lx); using full quality",what,(unsigned long)hr);Say(line);
    ready=false;return false;
}
ID3D12Resource* GetGazeMask(const State& state) {
    const auto& vp=state.vp;const float radius=gazeInner.load()+GazeQuantizationMargin;
    const int index=GazeCell(centerX.load(),centerY.load());
    std::lock_guard lock(maskLock);
    for(auto& b:gazeBanks)if(b.w==state.w&&b.h==state.h&&b.vx==vp.x&&b.vy==vp.y&&b.vw==vp.width&&b.vh==vp.height&&b.radius==radius)return b.textures[index].Get();
    // Bound resource lifetime without unsafe eviction of GPU-referenced images.
    if(gazeBanks.size()>=4){static bool warned=false;if(!warned){Say("Frame foveation: gaze viewport cache full; new viewports use full quality until restart");warned=true;}return nullptr;}
    const auto started=GetTickCount64();
    GazeBank b;b.w=state.w;b.h=state.h;b.vx=vp.x;b.vy=vp.y;b.vw=vp.width;b.vh=vp.height;b.radius=radius;
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=(b.w+tile-1)/tile;d.Height=(b.h+tile-1)/tile;
    d.DepthOrArraySize=d.MipLevels=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R8_UINT;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 bytes=0;
    nativeDevice->GetCopyableFootprints(&d,0,1,0,&fp,nullptr,nullptr,&bytes);
    const UINT64 stride=(bytes+D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT-1)&~UINT64(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT-1);
    D3D12_RESOURCE_DESC u{};u.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;u.Width=stride*GazeMaskCount;
    u.Height=u.DepthOrArraySize=u.MipLevels=u.SampleDesc.Count=1;u.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;
    if(!Hr(nativeDevice->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&u,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&b.upload)),"create gaze bank upload"))return nullptr;
    if(!Hr(nativeDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&b.allocator)),"create gaze bank allocator")||
       !Hr(nativeDevice->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,b.allocator.Get(),nullptr,IID_PPV_ARGS(&b.commands)),"create gaze bank list"))return nullptr;
    uint8_t* data=nullptr;D3D12_RANGE range{};
    if(!Hr(b.upload->Map(0,&range,reinterpret_cast<void**>(&data)),"map gaze bank"))return nullptr;
    std::memset(data,0,size_t(u.Width));heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    for(int i=0;i<GazeMaskCount;++i) {
        if(!Hr(nativeDevice->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&b.textures[i])),"create gaze rate image")){
            b.upload->Unmap(0,nullptr);return nullptr;
        }
        auto part=fp;part.Offset+=stride*i;
        for(uint32_t y=0;y<d.Height;++y)for(uint32_t x=0;x<d.Width;++x)
            data[part.Offset+y*part.Footprint.RowPitch+x]=FixedRate(x,y,tile,vp.x,vp.y,vp.width,vp.height,
                GazeGridCoordinate(i%GazeGrid),GazeGridCoordinate(i/GazeGrid),radius);
        D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=b.upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=part;
        dst.pResource=b.textures[i].Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;b.commands->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition={b.textures[i].Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,vrs::sourceState};b.commands->ResourceBarrier(1,&barrier);
    }
    b.upload->Unmap(0,nullptr);
    if(!Hr(b.commands->Close(),"close gaze bank list"))return nullptr;
    ID3D12CommandList* lists[]={b.commands.Get()};nativeQueue->ExecuteCommandLists(1,lists);
    char line[240];std::snprintf(line,sizeof line,"Frame foveation: gaze bank %dx%d ready for %ux%u viewport %.0fx%.0f, protected radius %.2f plus grid margin %.3f; setup %llu ms (one-time)",
        GazeGrid,GazeGrid,b.w,b.h,vp.width,vp.height,gazeInner.load(),GazeQuantizationMargin,static_cast<unsigned long long>(GetTickCount64()-started));Say(line);
    gazeBanks.push_back(std::move(b));return gazeBanks.back().textures[index].Get();
}
ID3D12Resource* GetMask(const State& state) {
    if(gazeDriven.load())return GetGazeMask(state);
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
    s.dirty=false;s.appliedEpoch=maskEpoch.load(std::memory_order_relaxed);
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
    ++pipelineGeneration;
}
void PipelineDestroyed(device*,pipeline p){std::unique_lock lock(pipelineLock);pipelines.erase(p.handle);++pipelineGeneration;}
void BindPipeline(command_list* cl,pipeline_stage stages,pipeline p){
    FRAME_PROFILE_SAMPLED(FoveationPipeline);
    if(uint32_t(stages&pipeline_stage::all_graphics)==0)return;
    auto& s=*Get(cl);const auto generation=pipelineGeneration.load(std::memory_order_relaxed);
    if(s.pipeline==p.handle&&s.pipelineEpoch==generation)return;
    bool eligible=false;
    {std::shared_lock lock(pipelineLock);const auto it=pipelines.find(p.handle);eligible=it!=pipelines.end()&&it->second.eligible;}
    if(s.eligible!=eligible){s.eligible=eligible;s.dirty=true;}
    s.pipeline=p.handle;s.pipelineEpoch=generation;
}
void BindTargets(command_list* cl,uint32_t count,const resource_view* rtvs,resource_view dsv){
    FRAME_PROFILE_SAMPLED(FoveationTargets);
    auto& s=*Get(cl);const auto oldW=s.w,oldH=s.h;s.targets=false;s.w=s.h=0;
    if(ready&&count>0&&count<=8&&rtvs&&rtvs[0].handle&&dsv.handle){
        auto* dev=cl->get_device();const auto color=dev->get_resource_desc(dev->get_resource_from_view(rtvs[0]));
        const auto depth=dev->get_resource_desc(dev->get_resource_from_view(dsv));
        s.w=color.texture.width;s.h=color.texture.height;
        s.targets=color.type==resource_type::texture_2d&&color.texture.samples==1&&depth.texture.width==s.w&&depth.texture.height==s.h&&
            s.w>=screenW*.45f&&s.h>=screenH*.45f&&std::abs(float(s.w)/s.h-float(screenW)/screenH)<.04f;
    }
    if(s.w!=oldW||s.h!=oldH)s.epoch=0;
    s.dirty=true;
}
bool BeginPass(command_list* cl,uint32_t count,const render_pass_render_target_desc* rts,const render_pass_depth_stencil_desc* ds,render_pass_flags){
    std::array<resource_view,8> rtvs{};for(uint32_t i=0;i<std::min(count,8u);++i)rtvs[i]=rts[i].view;
    BindTargets(cl,std::min(count,8u),rtvs.data(),ds?ds->view:resource_view{});return false;
}
bool EndPass(command_list* cl){auto& s=*Get(cl);s.targets=false;s.dirty=true;return false;}
void Viewports(command_list* cl,uint32_t first,uint32_t count,const viewport* vps){
    auto& s=*Get(cl);const auto old=s.vp;if(first==0&&count==1)s.vp=vps[0];else s.vp={};
    if(std::memcmp(&old,&s.vp,sizeof old)!=0){s.epoch=0;s.dirty=true;}
}
bool Draw(command_list* cl,uint32_t,uint32_t,uint32_t,uint32_t){
    FRAME_PROFILE_SAMPLED(FoveationDraw);
    auto& s=*Get(cl);
    // Bind callbacks only invalidate. Evaluate and set VRS once immediately
    // before the next draw, not again for every bind and every following draw.
    if(s.dirty||s.appliedEpoch!=maskEpoch.load(std::memory_order_relaxed))Apply(cl,s);
    // Diagnostic counts do not need a contended atomic on every game draw.
    // At most 255 pending draws per recording thread cross a sample boundary.
    static thread_local uint32_t shaded=0,full=0;
    if(s.hasMask)++shaded;else ++full;
    if(shaded+full>=256){shadedDraws.fetch_add(shaded,std::memory_order_relaxed);fullDraws.fetch_add(full,std::memory_order_relaxed);shaded=full=0;}
    return false;
}
bool DrawIndexed(command_list* cl,uint32_t,uint32_t,uint32_t,int32_t,uint32_t){return Draw(cl,0,0,0,0);}
bool Indirect(command_list* cl,indirect_command type,resource,uint64_t,uint32_t,uint32_t){
    if(type==indirect_command::draw||type==indirect_command::draw_indexed)return Draw(cl,0,0,0,0);
    // Unknown indirect signatures/mesh pipelines cannot be classified safely.
    auto& s=*Get(cl);const bool eligible=s.eligible;s.eligible=false;Apply(cl,s);s.eligible=eligible;s.dirty=true;return false;
}
bool Mesh(command_list* cl,uint32_t,uint32_t,uint32_t){return Indirect(cl,indirect_command::dispatch_mesh,{},0,0,0);}
void Reset(command_list* cl){auto& s=*Get(cl);s.eligible=s.targets=s.hasMask=false;s.w=s.h=0;s.vp={};s.applied=s.cached=nullptr;s.epoch=s.appliedEpoch=s.pipeline=s.pipelineEpoch=0;s.dirty=true;}
void Destroy(command_list* cl){
    uint64_t ptr=0;cl->get_private_data(reinterpret_cast<const uint8_t*>(&stateId),&ptr);
    cl->set_private_data(reinterpret_cast<const uint8_t*>(&stateId),0);
    ++stateGeneration;delete reinterpret_cast<State*>(ptr);
}
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
    reshade::register_event<reshade::addon_event::draw_or_dispatch_indirect>(&Indirect);
    reshade::register_event<reshade::addon_event::dispatch_mesh>(&Mesh);
    reshade::register_event<reshade::addon_event::reset_command_list>(&Reset);
    reshade::register_event<reshade::addon_event::destroy_command_list>(&Destroy);
    reshade::register_event<reshade::addon_event::reshade_begin_effects>(&Effects);
}
void Configure(bool on,float radius,float gazeRadius){
    const bool changed=enabled.exchange(on)!=on;
    const float value=std::isfinite(radius)?std::clamp(radius,.35f,.85f):.55f;
    const float gazeValue=std::isfinite(gazeRadius)?std::clamp(gazeRadius,.30f,.65f):.35f;
    const bool gazeChanged=gazeInner.exchange(gazeValue)!=gazeValue;
    if(inner.exchange(value)!=value||changed||gazeChanged)++maskEpoch;
}
void Initialize(ID3D12Device* dev,ID3D12CommandQueue* q,uint32_t w,uint32_t h){
    nativeDevice=dev;nativeQueue=q;screenW=w;screenH=h;vrs::Features f{};
    ready=SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6,&f,sizeof f))&&f.tier>=2&&(f.tile==8||f.tile==16||f.tile==32);
    tile=f.tile;++maskEpoch;char line[160];std::snprintf(line,sizeof line,"Frame foveation: fixed VRS %s; tier %u tile %u; 1x1 center / 2x2 opaque-world periphery",ready?"available":"unavailable",f.tier,tile);Say(line);
    Say("Frame foveation: cached command-list state, deferred rate binding, batched draw statistics");
}
void BeginEye(int e,float cx,float cy,bool tracked){eye=e;centerX=cx;centerY=cy;gazeDriven=tracked;++maskEpoch;worldDrawing=true;}
void EndWorld(){if(worldDrawing.exchange(false))++maskEpoch;}
void Suspend(command_list* cl){auto& s=*Get(cl);s.targets=false;Apply(cl,s);}
Stats TakeStats(){return {shadedDraws.exchange(0),fullDraws.exchange(0)};}
void Shutdown(){ready=false;worldDrawing=false;++maskEpoch;std::lock_guard lock(maskLock);masks.clear();gazeBanks.clear();nativeQueue.Reset();nativeDevice.Reset();}
}
