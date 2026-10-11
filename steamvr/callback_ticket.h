#pragma once
#include <cstdint>

namespace framevr {
// Caller owns synchronization. Loader callbacks may be discarded on a world
// transition. A lease permits recovery; a unique ticket makes late callbacks
// harmless instead of letting them consume a newer request's work.
class CallbackTicket {
    uintptr_t serial=0,pending=0;
    uint64_t since=0;
public:
    bool Expired(uint64_t now,uint64_t lease) const {return pending&&now-since>=lease;}
    uintptr_t Start(uint64_t now) {
        if(pending)return 0;
        if(!++serial)++serial;
        pending=serial;since=now;return pending;
    }
    bool Claim(uintptr_t token) {
        if(!token||token!=pending)return false;
        pending=0;return true;
    }
    void Cancel(){pending=0;}
    uint64_t Age(uint64_t now)const{return pending?now-since:0;}
};
}
