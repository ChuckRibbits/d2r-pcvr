#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

namespace framevr::bowflight {
// UI callbacks can request retirement, but must not delete HD entities while
// the renderer's registry is immutable. Consume requests from the native
// missile update/kill hooks. Keep the request until destruction returns so a
// reentrant UI callback cannot create its replacement halfway through cleanup.
// The caller serializes access. No game pointer is dereferenced here.
class ClientRetirements {
    struct Entry {
        uintptr_t unit;
        uint32_t id,missile;
        uint64_t ticket;
        bool destroying=false;
    };
    std::vector<Entry> entries;
    uint64_t serial=0;
public:
    bool Pending() const {return !entries.empty();}
    bool Contains(uintptr_t unit,uint32_t id,uint32_t missile) const {
        return std::any_of(entries.begin(),entries.end(),[&](const auto& e){return e.unit==unit&&e.id==id&&e.missile==missile;});
    }
    void Request(uintptr_t unit,uint32_t id,uint32_t missile) {
        if(unit&&!Contains(unit,id,missile))entries.push_back({unit,id,missile,++serial});
    }
    uint64_t Begin(uintptr_t unit,const uint32_t header[3]) {
        for(auto it=entries.begin();it!=entries.end();) {
            if(it->unit!=unit){++it;continue;}
            // A recycled address must never delete somebody else's missile.
            if(header[0]!=3||header[1]!=it->missile||header[2]!=it->id){it=entries.erase(it);continue;}
            if(it->destroying)return 0;
            it->destroying=true;return it->ticket;
        }
        return 0;
    }
    void Finish(uint64_t ticket) {
        if(!ticket)return;
        std::erase_if(entries,[&](const auto& e){return e.ticket==ticket&&e.destroying;});
    }
    void Reset(){entries.clear();} // serial survives: old callbacks cannot clear a new request
};
}
