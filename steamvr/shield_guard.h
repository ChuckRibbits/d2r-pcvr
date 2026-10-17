#pragma once
#include "frame_math.h"
#include <cstdint>

// Unleashed physical shield guard. Pure logic: no native types, no threads.
// The UI-thread tick feeds the shield hand and the posed shield; the game
// thread asks Judge() when a monster's native hit check has already succeeded
// against the local player.
namespace framevr::guard {
using math::Vec;
struct Tuning {
    // Block: the shield hand up in front of the chest. Measured from the head in
    // its turn only (the view, without pitch or roll): metres, x right, y up,
    // z back. The chest is under the eyes, not ahead of them: a shield held
    // against it reads -.16..+.03 forward, -.36..-.56 up (log 2026-10-16). A hand
    // hanging at the side (~0 forward, ~-.7 up) is what it refuses.
    float minForward=-.20f,maxForward=.85f;
    float minUp=-.62f,maxUp=.30f;          // ~navel to above the eyes; an arm hanging is -.69 and below
    float outward=.65f,across=.50f;        // to the shield's own side / across the body
    float hold=.05f;                       // a raised guard holds this much past every edge
    float coverCos=.5f;                    // 60 degree half-angle about the view, toward the attacker
    uint32_t parryMs=400;                  // 0 disables perfect parry; a swing at most this long before the hit
    // Parry: a swing across the view, low on the far side up to high on the
    // shield's own side, 45 degrees (a left shield: lower right -> upper left).
    float swingMetres=.15f;                // along the diagonal, within swingMs
    uint32_t swingMs=120;
    float swingCos=.82f;                   // ~35 degrees either side of the diagonal
    float parryRange=3.f;                  // metres; ranged attackers are blocked, not parried
    uint32_t staleMs=150;                  // pose older than this cannot block
};
struct State {
    bool raised=false,snapUsed=true;
    uint64_t raisedAt=0,loweredAt=0,seenAt=0;
    // Parry: the last diagonal swing that ended in guard; one parry per swing.
    uint64_t snapAt=0,swingSeen=0;
    // Diagnostic: the last melee hit blocked, to log a swing that came just after it.
    uint64_t blockAt=0;
    // the hand's recent path, one sample per >= 8 ms (render poses repeat between ticks)
    static constexpr int kPath=24;
    Vec path[kPath]{};uint64_t pathAt[kPath]{};int pathNext=0,pathCount=0;
    float swing=0,swingCos=0;  // last measured: metres along the diagonal, alignment
    Vec facing{}; // horizontal unit vector, where the view looks (render X/Z == grid X/Z axes)
    Vec hand{};   // last shield hand from the head, for [debug] guard_trace
    float forward=0,up=0,side=0;
};
enum class Verdict { Open, Block, Parry };

// The shield hand from the head (view frame): forward, up, and toward the
// shield's own side (+) or across the body (-).
inline void Axes(Vec hand,bool leftShield,float& forward,float& up,float& side) {
    forward=-hand.z;up=hand.y;side=leftShield?-hand.x:hand.x;
}
inline bool InGuard(Vec hand,bool leftShield,bool raised,const Tuning& t) {
    float f,u,s;Axes(hand,leftShield,f,u,s);
    const float m=raised?t.hold:0;
    return std::isfinite(f+u+s)&&f>=t.minForward-m&&f<=t.maxForward+m&&u>=t.minUp-m&&u<=t.maxUp+m&&
           s<=t.outward+m&&s>=-(t.across+m);
}

// hand: the shield hand from the head (view frame, metres); handOk false while
// it is not tracked. view: where the headset looks, on the ground, render X/Z
// (== grid X/Z axes): the way the guard faces. The hand is in that same frame,
// so a guard in front of the chest faces where the player looks. (From the
// hero's feet to the shield was noise for a guard held close: 2026-10-16.)
inline void Update(State& s,Vec hand,bool handOk,bool leftShield,Vec view,uint64_t now,bool posed,const Tuning& t) {
    if(!posed)return; // keep the last guard; Judge() expires it after staleMs
    const bool up=handOk&&InGuard(hand,leftShield,s.raised,t);
    if(handOk){s.hand=hand;Axes(hand,leftShield,s.forward,s.up,s.side);}
    if(up&&!s.raised)s.raisedAt=now;
    if(handOk&&std::isfinite(hand.x+hand.y+hand.z)) {
        const int last=(s.pathNext+State::kPath-1)%State::kPath;
        if(!s.pathCount||now<s.pathAt[last]||now-s.pathAt[last]>=8) {
            s.path[s.pathNext]=hand;s.pathAt[s.pathNext]=now;
            s.pathNext=(s.pathNext+1)%State::kPath;if(s.pathCount<State::kPath)++s.pathCount;
        }
        // the swing over the last swingMs: from the oldest sample still inside it
        int from=-1;
        for(int i=0;i<s.pathCount;++i) {
            const int k=(s.pathNext+State::kPath-s.pathCount+i)%State::kPath;
            if(now>=s.pathAt[k]&&now-s.pathAt[k]<=t.swingMs){from=k;break;}
        }
        s.swing=0;s.swingCos=0;
        if(from>=0) {
            float f0,u0,s0,f1,u1,s1;Axes(s.path[from],leftShield,f0,u0,s0);Axes(hand,leftShield,f1,u1,s1);
            const float ds=s1-s0,du=u1-u0,planar=std::hypot(ds,du);
            s.swing=(ds+du)*.70710678f;                 // along (side +1, up +1)
            s.swingCos=planar>1e-4f?s.swing/planar:0;
            if(up&&s.swing>=t.swingMetres&&s.swingCos>=t.swingCos) {
                // A new swing arms one parry; the rest of the same swing cannot re-arm it.
                // snapAt stays at the swing's start: the game thread spends it by that value.
                if(!s.swingSeen||now<s.swingSeen||now-s.swingSeen>t.swingMs){s.snapAt=now;s.snapUsed=false;}
                s.swingSeen=now;
            }
        }
    }
    if(!up&&s.raised)s.loweredAt=now;
    if(!s.loweredAt&&!up)s.loweredAt=now;
    s.raised=up;
    s.seenAt=now;
    if(std::isfinite(view.x+view.z)&&std::hypot(view.x,view.z)>.1f)s.facing=math::Normalized(Vec{view.x,0,view.z});
}

// toAttacker: grid-plane vector, player -> attacker, in metres. attackAt: when
// this attacker's attack began (0 = unknown). A parry is a swing while the
// attack is incoming: after it began, and at most parryMs before the hit.
inline Verdict Judge(const State& s,Vec toAttacker,uint64_t now,const Tuning& t,uint64_t attackAt=0) {
    if(!s.raised||!s.seenAt||now<s.seenAt||now-s.seenAt>t.staleMs)return Verdict::Open;
    const Vec flat{toAttacker.x,0,toAttacker.z};
    const float distance=std::hypot(flat.x,flat.z);
    // Standing on the attacker: any raised guard covers.
    if(distance>.05f&&math::Dot(s.facing,flat*(1/distance))<t.coverCos)return Verdict::Open;
    if(t.parryMs&&!s.snapUsed&&s.snapAt&&now>=s.snapAt&&now-s.snapAt<=t.parryMs&&distance<=t.parryRange&&
       (!attackAt||s.snapAt>=attackAt))
        return Verdict::Parry;
    return Verdict::Block;
}
// The game thread spends a swing on the parry it produced.
inline void SpendParry(State& s,uint64_t snapAt) {if(s.snapAt==snapAt)s.snapUsed=true;}

// Diagnostic only (a parry is never after the hit): a melee hit was just blocked.
inline void NoteBlock(State& s,uint64_t now) {s.blockAt=now;}
// UI thread, after Update: a swing that started within 400 ms after a blocked
// hit - too late to parry it - once, with how late it was. Spends nothing.
inline bool SwingAfterBlock(State& s,uint64_t now,const Tuning& t,uint64_t& late) {
    constexpr uint64_t kWindow=400;
    if(!s.blockAt)return false;
    if(now<s.blockAt||now-s.blockAt>kWindow+t.swingMs){s.blockAt=0;return false;}
    if(!s.snapAt||s.snapAt<s.blockAt||s.snapAt-s.blockAt>kWindow)return false;
    late=s.snapAt-s.blockAt;s.blockAt=0;return true;
}
}
