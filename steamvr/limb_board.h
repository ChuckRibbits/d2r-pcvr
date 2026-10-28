#pragma once
#include "limb_rig.h"
#include <atomic>
#include <mutex>

// Bodies that give way: what the skeleton hook's pose threads and the
// physics (the XR thread) hand each other. The hook posts each body near the
// player as posed and bends the ones the physics answers for.
namespace framevr::limbs {
inline constexpr unsigned MaxPosed=24;     // bodies within reach, posted at once
inline constexpr uint64_t FreshPose=200;   // ms: an older pose is not simulated
inline constexpr uint64_t FreshReaction=150; // ms: an older answer is not drawn
class Board {
    mutable std::mutex mutex;
    std::array<Rig,MaxPosed> rigs{};unsigned count=0;
    std::array<Reaction,MaxBodies> reactions{};unsigned answered=0;
    std::atomic<unsigned> reacting{0};
public:
    // The hook's counters, for the physics' log line.
    std::atomic<unsigned> posed{0},bent{0},refused{0},learned{0};
    // This body's pose, in place of its last; the stalest goes when full.
    void Pose(const Rig& r) {
        std::lock_guard lock(mutex);
        unsigned at=count;
        for(unsigned i=0;i<count;++i)if(rigs[i].entity==r.entity){at=i;break;}
        if(at==count) {
            if(count<MaxPosed)++count;
            else {at=0;for(unsigned i=1;i<count;++i)if(rigs[i].at<rigs[at].at)at=i;}
        }
        rigs[at]=r;
    }
    // The bodies posed within FreshPose ms of now; older ones are forgotten.
    unsigned Posed(Rig* out,unsigned max,uint64_t now) {
        std::lock_guard lock(mutex);
        unsigned kept=0,n=0;
        for(unsigned i=0;i<count;++i) {
            if(rigs[i].at>now||now-rigs[i].at>FreshPose)continue;
            rigs[kept++]=rigs[i];
            if(n<max)out[n++]=rigs[i];
        }
        count=kept;
        return n;
    }
    void React(const Reaction* r,unsigned n) {
        std::lock_guard lock(mutex);
        answered=std::min<unsigned>(n,MaxBodies);
        std::copy_n(r,answered,reactions.begin());
        reacting.store(answered,std::memory_order_release);
    }
    // The physics' answer for this body's pose of this model, if fresh.
    bool Answer(uint32_t entity,uint64_t layout,uint64_t now,Reaction& out) const {
        if(!reacting.load(std::memory_order_acquire))return false;
        std::lock_guard lock(mutex);
        for(unsigned i=0;i<answered;++i) {
            const auto& r=reactions[i];
            if(r.entity!=entity||r.layout!=layout||r.at>now||now-r.at>FreshReaction)continue;
            out=r;return true;
        }
        return false;
    }
    void Clear() {
        std::lock_guard lock(mutex);
        count=answered=0;reacting=0;
    }
};
inline Board& Shared() {static Board board;return board;}
}
