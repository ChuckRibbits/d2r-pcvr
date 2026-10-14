#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

namespace framevr::haptics {
// Some controller runtimes sustain vibration beyond the requested duration.
// Stop it explicitly on a timer independent of rendering. Apply, replacement,
// cancellation and expiry share a lock so an old timeout cannot stop a new tap.
// The owner must Stop() before destroying the captured XR session/action.
class Pulses {
    using Clock=std::chrono::steady_clock;
    struct Hand {bool active=false;Clock::time_point end{};};
    std::array<Hand,2> hands{};
    std::mutex mutex;
    std::condition_variable wake;
    std::thread worker;
    std::function<void(int)> stopPulse;
    bool stopped=true;
    void CancelLocked() {
        for(int i=0;i<2;++i)if(hands[i].active){hands[i].active=false;stopPulse(i);}
    }
    void Run() {
        std::unique_lock lock(mutex);
        while(!stopped) {
            auto next=Clock::time_point::max();
            for(const auto& hand:hands)if(hand.active)next=std::min(next,hand.end);
            if(next==Clock::time_point::max())wake.wait(lock);
            else wake.wait_until(lock,next);
            if(stopped)break;
            const auto now=Clock::now();
            for(int i=0;i<2;++i)if(hands[i].active&&now>=hands[i].end){hands[i].active=false;stopPulse(i);}
        }
    }
public:
    ~Pulses(){Stop();}
    // Start/Stop are serialized by the owner's session lifecycle lock.
    bool Start(std::function<void(int)> callback) {
        if(worker.joinable())return true;
        {std::lock_guard lock(mutex);stopPulse=std::move(callback);stopped=false;hands={};}
        try {worker=std::thread([this]{Run();});return true;}
        catch(...) {std::lock_guard lock(mutex);stopped=true;return false;}
    }
    template<class Apply> bool Pulse(int hand,std::chrono::nanoseconds duration,Apply&& apply) {
        std::lock_guard lock(mutex);
        if(stopped||hand<0||hand>=2||duration.count()<=0||!apply())return false;
        hands[hand]={true,Clock::now()+duration};wake.notify_one();return true;
    }
    void Cancel() {std::lock_guard lock(mutex);CancelLocked();wake.notify_one();}
    void Stop() {
        {std::lock_guard lock(mutex);stopped=true;CancelLocked();wake.notify_one();}
        if(worker.joinable())worker.join();
    }
};
}
