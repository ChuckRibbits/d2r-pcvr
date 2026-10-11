#include "memory.h"
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>
static std::atomic<bool> failed{false};
static double Run(bool old,unsigned threads,size_t size) {
    constexpr unsigned iterations=2000;
    std::atomic<unsigned> ready{0};std::atomic<bool> go{false};std::vector<std::thread> workers;
    for(unsigned t=0;t<threads;++t)workers.emplace_back([&]{
        std::vector<unsigned char> from(size,42),to(size,0);
        ++ready;while(!go.load(std::memory_order_acquire))SwitchToThread();
        for(unsigned i=0;i<iterations;++i) {
            SIZE_T copied=0;const bool ok=old?
                WriteProcessMemory(GetCurrentProcess(),to.data(),from.data(),size,&copied)&&copied==size:
                framevr::WriteMemory(to.data(),from.data(),size);
            if(!ok||to.back()!=42)failed=true;
        }
    });
    while(ready!=threads)SwitchToThread();
    const auto start=framevr::hookmetrics::Nanos();go.store(true,std::memory_order_release);
    for(auto& t:workers)t.join();
    return double(framevr::hookmetrics::Nanos()-start)/1000/iterations/threads;
}
int main(int argc,char** argv) {
    FILE* out=argc>1?std::fopen(argv[1],"w"):stdout;if(!out)return 1;
    std::fputs("Isolated Proton local-copy microbenchmark; wall microseconds/operation, not game frame savings.\n",out);
    for(unsigned threads:{1u,4u})for(size_t size:{size_t(12),size_t(64),size_t(8192)}) {
        const double old=Run(true,threads,size),local=Run(false,threads,size);
        std::fprintf(out,"threads=%u bytes=%zu wineserver=%.3f local=%.3f ratio=%.2f\n",threads,size,old,local,old/local);
    }
    if(out!=stdout)std::fclose(out);return failed?2:0;
}
