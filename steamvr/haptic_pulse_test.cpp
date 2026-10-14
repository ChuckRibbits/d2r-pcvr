#include "haptic_pulse.h"
#include <cstdio>
#include <cstdlib>
using namespace std::chrono_literals;
static void Check(bool value,const char* message){if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main() {
    framevr::haptics::Pulses pulses;
    std::mutex mutex;std::condition_variable changed;std::array<int,2> stops{};
    auto callback=[&](int hand){std::lock_guard lock(mutex);++stops[hand];changed.notify_all();};
    auto wait=[&](int hand,int count,auto timeout){std::unique_lock lock(mutex);return changed.wait_for(lock,timeout,[&]{return stops[hand]>=count;});};
    Check(pulses.Start(callback),"start timer");
    Check(pulses.Pulse(0,10ms,[]{return true;}),"send short impact");
    Check(wait(0,1,2s),"single impact stops even when no render/input update runs");
    Check(!wait(0,2,30ms),"one impact expires only once");
    Check(pulses.Pulse(0,40ms,[]{return true;}),"old pulse");
    Check(pulses.Pulse(0,200ms,[]{return true;}),"new pulse replaces deadline");
    Check(!wait(0,2,80ms),"old timeout cannot stop replacement pulse early");
    Check(pulses.Pulse(1,10ms,[]{return true;}),"other hand impact");
    Check(wait(1,1,2s)&&wait(0,2,2s),"hands expire independently");
    Check(!pulses.Pulse(0,10ms,[]{return false;}),"failed apply is not scheduled");
    Check(!wait(0,3,30ms),"failed pulse does not trigger later stop");
    pulses.Pulse(0,5s,[]{return true;});pulses.Pulse(1,5s,[]{return true;});
    pulses.Cancel();Check(wait(0,3,0ms)&&wait(1,2,0ms),"focus loss cancels both hands immediately");
    pulses.Pulse(0,5s,[]{return true;});pulses.Stop();
    Check(wait(0,4,0ms),"shutdown stops active haptic before session destruction");
    Check(!pulses.Pulse(0,10ms,[]{return true;}),"no calls after shutdown");
    Check(pulses.Start(callback)&&pulses.Pulse(1,10ms,[]{return true;}),"new session can restart");
    Check(wait(1,3,2s),"restarted timer expires");pulses.Stop();
    std::puts("Haptics: explicit expiry, replacement, separate hands, cancellation and session lifetime passed");
}
