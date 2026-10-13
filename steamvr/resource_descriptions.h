#pragma once
#include "generation_cache.h"
#include "hook_metrics.h"
#include <reshade.hpp>
#include <atomic>

namespace framevr::resources {
using namespace reshade::api;
// Cache immutable RESOURCE descriptions, never descriptor -> resource mappings:
// D3D12 CopyDescriptors can replace an RTV/DSV without init_resource_view.
// Keep resolving each view through ReShade so descriptor copies remain visible.
inline std::atomic<uint64_t> generation{1};
inline bool registered=false;
inline void Created(device*,const resource_desc&,const subresource_data*,resource_usage,resource) {
    generation.fetch_add(1,std::memory_order_release);
}
inline void Destroyed(device*,resource) {generation.fetch_add(1,std::memory_order_release);}
inline void DeviceDestroyed(device*) {generation.fetch_add(1,std::memory_order_release);}
inline void Register() {
    if(registered)return;
    reshade::register_event<reshade::addon_event::init_resource>(&Created);
    reshade::register_event<reshade::addon_event::destroy_resource>(&Destroyed);
    reshade::register_event<reshade::addon_event::destroy_device>(&DeviceDestroyed);
    registered=true;
}

class Descriptions {
    device* owner=nullptr;
    GenerationCache<resource_desc,128> cache;
public:
    template<class Read> resource_desc Get(device* dev,resource target,Read&& read) {
        if(!target.handle)return {};
        if(owner!=dev){owner=dev;cache={};}
        const auto epoch=generation.load(std::memory_order_acquire);
        resource_desc desc;
        if(!cache.Get(target.handle,epoch,desc)) {
            hookmetrics::Scope measured(hookmetrics::ResourceDescription,63);
            desc=read(target);cache.Put(target.handle,epoch,desc);
        }
        return desc;
    }
};
inline resource_desc Describe(device* dev,resource target) {
    static thread_local Descriptions descriptions;
    return descriptions.Get(dev,target,[&](resource r){return dev->get_resource_desc(r);});
}
inline resource Resolve(device* dev,resource_view view) {
    if(!view.handle)return {};
    hookmetrics::Scope measured(hookmetrics::ResourceViewResolve,63);
    return dev->get_resource_from_view(view);
}
} // namespace framevr::resources
