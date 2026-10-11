#include "latest_worker.h"
#include "interaction_paint.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>
using namespace std::chrono_literals;
static void Check(bool b,const char* s){if(!b){std::fprintf(stderr,"FAIL: %s\n",s);std::exit(1);}}
struct Gate {
    std::mutex mutex;std::condition_variable wake;bool open=false;
    void Open(){std::lock_guard lock(mutex);open=true;wake.notify_all();}
    void Wait(){std::unique_lock lock(mutex);wake.wait(lock,[&]{return open;});}
};
template<class F> void Until(F predicate) {
    const auto end=std::chrono::steady_clock::now()+3s;
    while(!predicate()){Check(std::chrono::steady_clock::now()<end,"bounded worker completion");std::this_thread::sleep_for(1ms);}
}
int main() {
    Gate entered,release;
    std::atomic<int> calls=0;
    framevr::LatestWorker<int,std::vector<int>> worker([&](const int& i){
        ++calls;if(i==1){entered.Open();release.Wait();}
        if(i<0)throw std::runtime_error("test failure");return std::vector<int>{i};
    });
    Check(worker.Submit(7)==0,"stopped worker rejects requests");Check(worker.Start(),"start");
    const auto first=worker.Submit(1);entered.Wait();
    uint64_t latest=0;for(int i=2;i<=1000;++i)latest=worker.Submit(i);
    std::vector<int> output;
    Check(!worker.Take(first,output),"busy producer does not block consumer");
    release.Open();Until([&]{return worker.Take(latest,output);});
    Check(output==std::vector<int>{1000}&&calls==2,"only latest pending job runs; completed obsolete image is discarded");
    Check(!worker.Take(first,output)&&!worker.Take(latest,output),"no stale result and each result taken once");
    auto bad=worker.Submit(-1);Until([&]{return worker.Take(bad,output);});Check(output.empty(),"failure safely returns empty result");
    worker.Stop();Check(!worker.Take(bad,output)&&worker.Submit(5)==0,"stop clears mailbox");
    Check(worker.Start(),"restart");auto next=worker.Submit(2);Check(next>latest,"restart cannot reuse a result identity");
    Until([&]{return worker.Take(next,output);});Check(output==std::vector<int>{2},"restart accepts new work");worker.Stop();
    Gate running,finish;std::atomic<bool> stopReturned=false;
    framevr::LatestWorker<int,int> stopping([&](const int& i){running.Open();finish.Wait();return i;});
    Check(stopping.Start(),"start blocking worker");const auto pending=stopping.Submit(3);running.Wait();
    std::thread stop([&]{stopping.Stop();stopReturned=true;});
    std::this_thread::sleep_for(5ms);Check(!stopReturned,"stop joins in-flight work before resource teardown");finish.Open();stop.join();
    int unused=0;Check(!stopping.Take(pending,unused),"shutdown completion cannot reappear");
    using namespace framevr::interaction;Snapshot s;s.session=9;Output o;
    const auto context=Context(s,o,true);s.health=23;s.mana=12;++s.revision;
    Check(context==Context(s,o,true),"vitals-only refresh cannot blank the wheel or block its clicks");
    o.hover=4;o.nearHolster=1;Check(context==Context(s,o,true),"hover and body proximity do not change clickable contents");
    o.editor=2;Check(context!=Context(s,o,true),"pending editor page cannot expose old clickable layout");o.editor=0;
    s.skills[4].id=70;Check(context!=Context(s,o,true),"pending reassignment cannot expose old spell");s.skills[4].id=-1;
    ++s.session;Check(context!=Context(s,o,true),"previous character image cannot be clicked");--s.session;
    Check(context!=Context(s,o,false),"stale native snapshot cannot use a fresh image for clicks");
    std::puts("Latest-only worker: coalescing, nonblocking consumer, failures, restart, shutdown and clickable-content guards passed");
}
