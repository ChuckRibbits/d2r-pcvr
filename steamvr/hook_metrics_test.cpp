#include "hook_metrics.h"
#include "generation_cache.h"
#include "native_lock.h"
#include <cstdlib>
#include <cstdio>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>
using namespace framevr;
static void Check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
static uint64_t Sum(const hookmetrics::Snapshot& s,hookmetrics::Kind kind) {
    uint64_t n=0;for(const auto& t:s)n+=t.counters[kind].calls;return n;
}
int main() {
    {hookmetrics::Scope disabled(hookmetrics::MemoryWrite,0,64);}
    Check(hookmetrics::allocated==0,"disabled diagnostics do not even allocate a thread slot");
    auto baseline=hookmetrics::Start(1);
    std::atomic<bool> done=false;
    std::thread reader([&]{while(!done){auto snapshot=hookmetrics::Read();(void)snapshot;}});
    std::vector<std::thread> workers;
    for(int t=0;t<8;++t)workers.emplace_back([]{
        for(int i=0;i<10000;++i){hookmetrics::Scope scope(hookmetrics::MemoryWrite,63,64);}
    });
    for(auto& t:workers)t.join();done=true;reader.join();hookmetrics::Stop();
    auto first=hookmetrics::Read();
    Check(Sum(first,hookmetrics::MemoryWrite)-Sum(baseline,hookmetrics::MemoryWrite)==80000,"all worker calls included without shared counter contention");
    unsigned observed=0;
    for(const auto& t:first)if(t.id){++observed;const auto& c=t.counters[hookmetrics::MemoryWrite];
        Check(c.calls==10000&&c.bytes==640000&&c.samples==156,"independent per-thread counts, bytes and sample cadence");}
    Check(observed==8,"workers remain identifiable after thread exit");
    {hookmetrics::Scope disabled(hookmetrics::MemoryWrite,0);}
    Check(Sum(first,hookmetrics::MemoryWrite)==Sum(hookmetrics::Read(),hookmetrics::MemoryWrite),"stopped counters idle");
    baseline=hookmetrics::Start(2);
    {
        hookmetrics::Scope crossing(hookmetrics::MemoryRead,0,16);
        hookmetrics::Stop();hookmetrics::Start(3);
    }
    for(const auto& t:hookmetrics::Read())Check(t.counters[hookmetrics::MemoryRead].samples==0,"old timed scope cannot contaminate a new capture");
    hookmetrics::Stop();
    GenerationCache<bool,2> cache;bool value=false;
    Check(!cache.Get(0,0,value),"empty cache does not match zero-valued key/generation");
    cache.Put(0,1,false);Check(cache.Get(0,1,value)&&!value,"negative lookup cached");
    cache.Put(0,2,true);Check(!cache.Get(0,1,value)&&cache.Get(0,2,value)&&value,"creation or handle reuse invalidates old result");
    cache.Put(2,2,false);Check(!cache.Get(0,2,value)&&cache.Get(2,2,value)&&!value,"colliding keys cannot inherit another pipeline classification");
    NativeMutex mutex;uint64_t sum=0;workers.clear();
    for(int t=0;t<8;++t)workers.emplace_back([&]{for(int i=0;i<10000;++i){std::lock_guard lock(mutex);++sum;}});
    for(auto& t:workers)t.join();
    {std::shared_lock lock(mutex);Check(sum==80000,"native callback lock serializes writes and supports shared reads");}
    std::puts("All-thread counters, capture boundaries, generation invalidation and native lock stress passed");
}
