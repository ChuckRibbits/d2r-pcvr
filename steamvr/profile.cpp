#include "profile.h"
#include "logging.h"
#include <atomic>
#include <cstdio>

namespace framevr::profile {
thread_local Counters* current=nullptr;
namespace {
std::atomic<uint32_t> requested{0};
std::atomic<bool> gpuEnabled{true};
uint32_t sequence=0,remaining=0,frames=0;
bool active=false;
Counters eyes[2],totals[2];
const char* names[Count]={"camera-game","camera-sync","view","projection","skeleton-game","skeleton-mod",
    "vrs-pipeline","vrs-targets","vrs-draw","hud-bind","depth-bind","fx-setup","fx-finish"};
}
double Us(){static const double scale=[](){LARGE_INTEGER f;QueryPerformanceFrequency(&f);return 1e6/f.QuadPart;}();LARGE_INTEGER t;QueryPerformanceCounter(&t);return t.QuadPart*scale;}
void Request(uint32_t n){static uint32_t last=0;if(n&&n!=last)requested.store(n);last=n;}
void ConfigureGpu(bool on){gpuEnabled.store(on,std::memory_order_relaxed);}
bool GpuEnabled(){return gpuEnabled.load(std::memory_order_relaxed);}
void BeginPair(){
    if(const auto next=requested.exchange(0)){sequence=next;remaining=360;frames=0;totals[0]={};totals[1]={};logging::Write("Frame profile: start 360 gameplay pairs; main-render-thread inclusive scopes, high-frequency callbacks sampled 1/64; GPU scene spans include queue starvation");}
    active=remaining>0;current=nullptr;eyes[0]={};eyes[1]={};
}
bool Active(){return active;}
void SkipPair(){active=false;current=nullptr;}
void BeginEye(unsigned eye){current=active?&eyes[eye&1]:nullptr;}
void FinishPair(double gap,double begin,double left,double right,double end,double cpuL,double cpuR,const FrameTiming& x){
    current=nullptr;if(!active)return;
    ++frames;--remaining;
    char line[640];
    std::snprintf(line,sizeof line,"Frame profile,%u,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u",
        sequence,frames,gap,begin,left,right,end,cpuL,cpuR,x.waitFrameMs,x.locateMs,x.inputMs,
        x.acquireMs[0],x.acquireMs[1],x.copySubmitMs[0],x.copySubmitMs[1],x.waitGpuMs,x.releaseMs,x.submitMs,
        x.gpuSceneMs[0],x.gpuSceneMs[1],x.gpuEffectsMs[0],x.gpuEffectsMs[1],x.gpuCopyMs[0],x.gpuCopyMs[1],x.gpuValid?1:0);
    logging::Write(line);
    for(int e=0;e<2;++e)for(int b=0;b<Count;++b){auto& t=totals[e][b];const auto& s=eyes[e][b];t.calls+=s.calls;t.samples+=s.samples;t.us+=s.us;}
    if(frames%120==0){
        for(int e=0;e<2;++e)for(int b=0;b<Count;++b){const auto& c=totals[e][b];
            std::snprintf(line,sizeof line,"Frame scope,%u,%u,%d,%s,%llu,%llu,%.3f",sequence,frames,e,names[b],(unsigned long long)c.calls,(unsigned long long)c.samples,c.samples?c.us*c.calls/c.samples/frames/1000:0);
            logging::Write(line);
        }
    }
    if(!remaining){active=false;logging::Write("Frame profile: complete; diagnostic scopes and GPU queries idle");}
}
}
