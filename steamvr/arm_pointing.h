#pragma once
#include <algorithm>

namespace framevr::armpointing {
// Is the aiming hand held out to point? The hand's horizontal reach from the
// head (metres) against the feature's "arm out" distance (cm); it lets go
// 6 cm nearer, so the answer does not flicker at the edge. Pointing steeply
// down (at something by the feet) the arm cannot reach as far, so 3/4 of the
// distance counts. 0 cm: the hand always points. A feature keeps its own State.
inline constexpr float ReleaseM=.06f,DownDip=-.75f,DownShare=.75f;
// The arm counts as pointing once held out a tenth of the player's span (Jeff:
// 15 cm on a 1.5 m span): VR Settings > Controller span, metres, to the arm-out cm.
inline constexpr float SpanShare=.1f;
inline float ArmOutCm(float spanM){return spanM*SpanShare*100.f;}
struct State {bool on=false;};
inline bool Update(State& state,float reachM,float rayY,float armOutCm) {
    const float on=armOutCm*.01f,off=std::max(0.f,on-ReleaseM);
    const float need=(state.on?off:on)*(rayY<DownDip?DownShare:1.f);
    state.on=on<=0||reachM>=need;
    return state.on;
}
}
