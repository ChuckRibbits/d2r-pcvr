#pragma once
// Logging-only traces for the aim rules (PR #254): the bookkeeping, kept pure
// so it can be tested. Nothing here reads the game.
//  - "Frame hand-off miss:" (vrcam): Bone Spirit and the other unit hand-offs
//    whose monster the client rooms did not list, at most one a second.
//  - "Frame wall trace:" (projectiles): what a Bone Wall or Bone Prison cast
//    leaves near its cast point, for a few seconds, a few casts a game.
#include <cmath>
#include <cstdint>

namespace framevr::aimtrace {
// At most one per gap: true (and the clock reset) when due.
inline bool Due(uint64_t& last,uint64_t now,uint64_t gap) {
    if(last&&now-last<gap)return false;
    last=now;return true;
}
constexpr uint64_t MissGapMs=1000;

// The wall window: opened by a cast of Bone Wall (78) or Bone Prison (88), it
// lasts WindowMs, samples at most every SampleMs, and at most CastsPerGame
// casts a game are traced (a new game key resets the count).
constexpr uint64_t WindowMs=6000,SampleMs=100;
constexpr int CastsPerGame=3;
constexpr float NearCells=10;   // logged around the cast point
constexpr float BaseCells=20;   // already there at the cast: not "new"
inline bool WallSkill(int skill){return skill==78||skill==88;}
struct Window {
    uint64_t start=0,sampled=0,game=0;
    int skill=-1,used=0;
    float x=0,y=0;
    bool Open(uint64_t now)const{return start&&now-start<WindowMs;}
    // A new game: no window, the cast budget back.
    void Game(uint64_t key){if(key!=game){*this={};game=key;}}
    // A cast: opens the window unless one is open or the budget is spent.
    bool Arm(int s,uint64_t now,float px,float py) {
        if(!WallSkill(s)||!now||Open(now)||used>=CastsPerGame)return false;
        start=now;sampled=0;skill=s;x=px;y=py;++used;return true;
    }
    // Inside the window and not sampled within SampleMs.
    bool SampleDue(uint64_t now) {
        if(!Open(now))return false;
        return Due(sampled,now,SampleMs);
    }
    // The first call after the window closed: true once, then shut.
    bool Closed(uint64_t now) {
        if(!start||Open(now))return false;
        start=0;return true;
    }
};
inline bool Within(float x,float y,float cx,float cy,float cells) {
    return std::hypot(x-cx,y-cy)<=cells;
}

// Units seen near the cast: the ones present at the cast (baseline), and the
// ones that came after (new), each new one reported once when it appears and
// once when it is gone.
struct Unit {uint32_t type=0,id=0;bool base=false,seen=false;uint64_t at=0;float x=0,y=0;};
template<int N> struct Tracker {
    Unit units[N]{};
    int count=0;
    void Clear(){*this={};}
    int Find(uint32_t type,uint32_t id)const {
        for(int i=0;i<count;++i)if(units[i].type==type&&units[i].id==id)return i;
        return -1;
    }
    // Before each sample.
    void Begin(){for(int i=0;i<count;++i)units[i].seen=false;}
    // Already there at the cast (the first sample): never reported as new.
    void Base(uint32_t type,uint32_t id) {
        if(Find(type,id)<0&&count<N)units[count++]={type,id,true,true,0};
    }
    // Seen in this sample, at x,y (its last place is kept). True: new (report
    // it). Full: neither kept nor reported.
    bool Seen(uint32_t type,uint32_t id,uint64_t now,float x=0,float y=0) {
        const int i=Find(type,id);
        if(i>=0){units[i].seen=true;units[i].x=x;units[i].y=y;return false;}
        if(count>=N)return false;
        units[count++]={type,id,false,true,now,x,y};return true;
    }
    // The new units not seen in this sample: f(unit) each, then forgotten.
    template<class F> int Gone(F f) {
        int n=0;
        for(int i=0;i<count;) {
            if(!units[i].seen&&!units[i].base){f(units[i]);units[i]=units[--count];++n;}
            else ++i;
        }
        return n;
    }
};
}
