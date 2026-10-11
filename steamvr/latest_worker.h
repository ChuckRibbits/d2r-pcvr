#pragma once
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace framevr {
// One running job, one replaceable pending job and one finished result. Work
// owns its input and runs outside the mailbox lock. Callers serialize Start/Stop
// with Submit/Take. The callback must never access game/XR lifetime or locks.
template<class Input,class Output> class LatestWorker {
    struct Job {uint64_t id;Input input;};
    struct Result {uint64_t id;Output output;};
    std::mutex mutex;
    std::condition_variable wake;
    std::thread worker;
    std::optional<Job> pending;
    std::optional<Result> ready;
    uint64_t generation=0;
    bool stop=true;
    std::function<Output(const Input&)> work;
    void Run() {
        for(;;) {
            std::optional<Job> job;
            {std::unique_lock lock(mutex);wake.wait(lock,[&]{return stop||pending.has_value();});
             if(stop)return;job=std::move(pending);pending.reset();}
            Output output{};
            try {output=work(job->input);}catch(...) {}
            {std::lock_guard lock(mutex);
             if(!stop&&job->id==generation)ready=Result{job->id,std::move(output)};}
        }
    }
public:
    explicit LatestWorker(std::function<Output(const Input&)> fn):work(std::move(fn)){}
    ~LatestWorker(){Stop();}
    LatestWorker(const LatestWorker&)=delete;
    LatestWorker& operator=(const LatestWorker&)=delete;
    bool Start() {
        if(worker.joinable())return true;
        {std::lock_guard lock(mutex);stop=false;}
        try {worker=std::thread([this]{Run();});return true;}
        catch(...) {std::lock_guard lock(mutex);stop=true;return false;}
    }
    uint64_t Submit(Input input) {
        std::lock_guard lock(mutex);if(stop)return 0;
        const auto id=++generation;ready.reset();pending=Job{id,std::move(input)};wake.notify_one();return id;
    }
    bool Take(uint64_t id,Output& output) {
        std::lock_guard lock(mutex);
        if(!ready||!id||ready->id!=id||generation!=id)return false;
        output=std::move(ready->output);ready.reset();return true;
    }
    void Stop() {
        {std::lock_guard lock(mutex);stop=true;++generation;pending.reset();ready.reset();wake.notify_one();}
        if(worker.joinable())worker.join();
    }
};
}
