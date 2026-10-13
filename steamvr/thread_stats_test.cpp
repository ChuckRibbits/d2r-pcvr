#include "thread_stats.h"
#include <cassert>
#include <chrono>
#include <thread>
#include <cstring>
using namespace framevr::threadstats;
using namespace std::chrono_literals;
template<class F> void Until(F predicate) {
    const auto deadline=std::chrono::steady_clock::now()+3s;
    while(!predicate()){assert(std::chrono::steady_clock::now()<deadline);std::this_thread::sleep_for(1ms);}
}
int main(int argc,char** argv) {
    Snapshot before{{{1,{100,200}},{2,{101,300}},{3,{102,500}},{4,{103,200}}},1000000,0,true};
    Snapshot after{{{1,{100,5000200}},{2,{101,2500300}},{3,{999,9000500}},{4,{103,100}},{5,{104,9999999}}},2000000,0,true};
    assert(Format(before,after,1)=="1 (draw) 50%, 2 25%"); // elapsed CPU, no turbo calibration
    assert(Format({},after,1)=="collecting baseline");
    assert(Format(before,{},1)=="thread counters unavailable");
    after.atUs=before.atUs;assert(Format(before,after,1)=="collecting baseline");after.atUs=2000000;
    Snapshot none{{},2000000,0,true};assert(Format(before,none,1)=="no comparable live threads");
    // Block the actual scanner: repeated render-side polls must remain usable
    // without entering it, and shutdown must finish before callback data dies.
    std::mutex mutex;std::condition_variable wake;bool entered=false,released=false;
    const auto caller=GetCurrentThreadId();DWORD scanner=0;
    Sampler sampler([&]{
        std::unique_lock lock(mutex);scanner=GetCurrentThreadId();entered=true;wake.notify_one();
        wake.wait(lock,[&]{return released;});return after;
    });
    assert(sampler.Submit(caller)==0);assert(sampler.Start());const auto request=sampler.Submit(caller);
    {std::unique_lock lock(mutex);assert(wake.wait_for(lock,3s,[&]{return entered;}));}
    assert(scanner!=caller);Report report;
    for(int i=0;i<1000;++i)assert(!sampler.Take(request,report));
    {std::lock_guard lock(mutex);released=true;wake.notify_one();}
    Until([&]{return sampler.Take(request,report);});assert(report.text=="collecting baseline");
    const auto next=sampler.Submit(caller);Until([&]{return sampler.Take(next,report);});
    sampler.Stop();assert(sampler.Submit(caller)==0&&!sampler.Take(next,report));
    assert(sampler.Start());const auto restarted=sampler.Submit(caller);
    Until([&]{return sampler.Take(restarted,report);});assert(report.text=="collecting baseline");sampler.Stop();
    const auto live=Collect();assert(live.valid&&live.threads.count(caller));
    if(argc>1&&std::strcmp(argv[1],"--benchmark")==0) {
        std::vector<double> times;for(int i=0;i<5;++i)times.push_back(Collect().scanMs);
        std::sort(times.begin(),times.end());
        std::printf("Synchronous system thread scan: min %.3f, median %.3f, max %.3f ms\n",times.front(),times[2],times.back());
        Sampler native;assert(native.Start());const double start=Us();const auto id=native.Submit(caller);const double submitted=Us()-start;
        Until([&]{return native.Take(id,report);});native.Stop();
        std::printf("Background scan %.3f ms; render-side submission %.3f us\n",report.scanMs,submitted);
    }
    std::puts("Thread stats: CPU percentages, thread ID reuse, unavailable samples, asynchronous scan and restart passed");
}
