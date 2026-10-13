#pragma once
#include "latest_worker.h"
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace framevr::threadstats {
struct Sample {uint64_t created=0,cpu=0;}; // FILETIME ticks, 100 ns
struct Snapshot {
    std::unordered_map<DWORD,Sample> threads;
    double atUs=0,scanMs=0;
    bool valid=false;
};
inline double Us() {
    static const double scale=[](){LARGE_INTEGER f;QueryPerformanceFrequency(&f);return 1e6/f.QuadPart;}();
    LARGE_INTEGER n;QueryPerformanceCounter(&n);return n.QuadPart*scale;
}
inline uint64_t Ticks(const FILETIME& t){return (uint64_t(t.dwHighDateTime)<<32)|t.dwLowDateTime;}
// Toolhelp takes a system-wide snapshot even though only this process's threads
// are retained. It must never run in Present, input, or a game callback.
inline Snapshot Collect() {
    Snapshot result;result.atUs=Us();
    const HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(snapshot==INVALID_HANDLE_VALUE)return result;
    THREADENTRY32 entry{sizeof entry};const DWORD pid=GetCurrentProcessId();
    BOOL more=Thread32First(snapshot,&entry);
    while(more) {
        if(entry.th32OwnerProcessID==pid) {
            const HANDLE thread=OpenThread(THREAD_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ThreadID);
            if(thread) {
                FILETIME created{},exited{},kernel{},user{};
                if(GetThreadTimes(thread,&created,&exited,&kernel,&user))
                    result.threads.emplace(entry.th32ThreadID,Sample{Ticks(created),Ticks(kernel)+Ticks(user)});
                CloseHandle(thread);
            }
        }
        more=Thread32Next(snapshot,&entry);
    }
    const DWORD error=GetLastError();CloseHandle(snapshot);
    result.scanMs=(Us()-result.atUs)*.001;
    result.valid=error==ERROR_NO_MORE_FILES;
    return result;
}
inline std::string Format(const Snapshot& previous,const Snapshot& current,DWORD drawThread) {
    if(!current.valid)return "thread counters unavailable";
    if(!previous.valid||current.atUs<=previous.atUs)return "collecting baseline";
    std::vector<std::pair<double,DWORD>> busy;
    const double spanUs=current.atUs-previous.atUs;
    for(const auto& [id,sample]:current.threads) {
        const auto found=previous.threads.find(id);
        if(found==previous.threads.end()||found->second.created!=sample.created||sample.cpu<found->second.cpu)continue;
        busy.emplace_back(10.0*double(sample.cpu-found->second.cpu)/spanUs,id);
    }
    std::sort(busy.begin(),busy.end(),[](const auto& a,const auto& b){return a.first!=b.first?a.first>b.first:a.second<b.second;});
    std::string result;char text[80];
    for(size_t i=0;i<busy.size()&&i<5;++i) {
        std::snprintf(text,sizeof text,"%s%lu%s %.0f%%",i?", ":"",busy[i].second,busy[i].second==drawThread?" (draw)":"",busy[i].first);
        result+=text;
    }
    return result.empty()?"no comparable live threads":result;
}
struct Report {std::string text;double scanMs=0;};
class Sampler {
    std::function<Snapshot()> collect;
    Snapshot previous;
    LatestWorker<DWORD,Report> worker;
public:
    explicit Sampler(std::function<Snapshot()> fn=Collect):collect(std::move(fn)),worker([this](DWORD drawThread){
        auto current=collect();Report result{Format(previous,current,drawThread),current.scanMs};
        if(current.valid)previous=std::move(current);return result;
    }){}
    bool Start(){return worker.Start();}
    void Stop(){worker.Stop();previous={};}
    uint64_t Submit(DWORD drawThread){return worker.Submit(drawThread);}
    bool Take(uint64_t request,Report& result){return worker.Take(request,result);}
};
}
