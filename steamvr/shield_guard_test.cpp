#include "shield_guard.h"
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace framevr::guard;
using framevr::math::Vec;

// Default fixture: shield faces ahead (-Z). The production path has no view
// argument: these helpers rotate both the held shield and hand for old gestures.
static const Vec view{0,0,-1},ahead{0,0,-1.5f};
static Pose Surface(Vec hand,Vec facing,uint64_t at) {
    const Vec right=framevr::math::Cross(facing,{0,1,0});
    const Vec eye{0,1.7f,0};
    return {eye+right*hand.x+Vec{0,hand.y,0}+facing*(-hand.z),facing,eye,hand,4,at,1};
}
static void Update(State& s,Vec hand,bool handOk,bool left,Vec facing,uint64_t now,bool posed,const Tuning& t) {
    framevr::guard::Update(s,Surface(hand,facing,now),handOk,left,now,posed,t);
}

// Feed the hand (head frame) along a straight path from a to b over ms, one
// pose per 11 ms frame but sampled every 1 ms tick, as vrcam's timer does.
static void Move(State& s,Vec a,Vec b,bool left,uint64_t& now,uint64_t ms,const Tuning& t) {
    for(uint64_t i=1;i<=ms;++i) {
        const uint64_t frame=i-(i%11);
        const float f=float(frame)/float(ms);
        Update(s,a+(b-a)*f,true,left,view,++now,true,t);
    }
    Update(s,b,true,left,view,++now,true,t);
}
static void Hold(State& s,Vec p,bool left,uint64_t& now,uint64_t ms,const Tuning& t) {
    for(uint64_t i=0;i<ms;++i)Update(s,p,true,left,view,++now,true,t);
}

int main() {
    const Tuning t;
    // Head frame: x right, y up, z back (metres from the eyes). A left shield.
    const Vec hanging{-.22f,-.72f,-.02f};   // arm down at the side
    const Vec chest{-.12f,-.35f,-.25f};     // held close to the chest
    const Vec high{-.15f,.10f,-.35f};       // up at the face
    const Vec tucked{.05f,-.30f,-.14f};     // tight in, toward the middle
    const Vec far{-.10f,-.25f,-.70f};       // pushed well out ahead
    const Vec against{-.12f,-.45f,.14f};    // flat on the chest: behind the eyes (log 2026-10-16)
    const Vec againstLow{-.09f,-.56f,.05f};
    const Vec low{-.10f,-.66f,-.35f};       // ahead but down at the belt
    const Vec wide{-.80f,-.30f,-.20f};      // out to the left side
    const Vec swingFrom{.18f,-.48f,-.30f};  // lower right
    const Vec swingTo{-.20f,-.06f,-.32f};   // upper left
    State s;uint64_t now=1000;

    // Hanging at the side: open.
    Hold(s,hanging,true,now,40,t);
    assert(!s.raised&&Judge(s,ahead,now,t)==Verdict::Open);
    // Anywhere a shield naturally covers the chest blocks; a slow raise is no parry.
    for(Vec p:{chest,high,tucked,far,against,againstLow}) {
        State g;uint64_t n=2000;Hold(g,hanging,true,n,40,t);Move(g,hanging,p,true,n,700,t);
        assert(g.raised&&Judge(g,ahead,n,t)==Verdict::Block);
    }
    // Down at the belt or held off the incoming line: open.
    for(Vec p:{low,wide}) {State g;uint64_t n=3000;Hold(g,p,true,n,40,t);assert(Judge(g,ahead,n,t)==Verdict::Open);}
    // A raised guard holds a few cm past the edge (no flicker at the line).
    {State g;uint64_t n=4000;Hold(g,chest,true,n,40,t);Hold(g,Vec{-.12f,-.66f,-.25f},true,n,40,t);assert(g.raised);
     Hold(g,Vec{-.12f,-.70f,-.25f},true,n,40,t);assert(!g.raised);}
    // Directional surface: ahead blocks, attackers beside/behind this shield do not.
    Move(s,hanging,chest,true,now,700,t);
    assert(Judge(s,{.8f,0,-1.f},now,t)==Verdict::Block);
    assert(Judge(s,{1.5f,0,0},now,t)==Verdict::Open);
    assert(Judge(s,{0,0,1.5f},now,t)==Verdict::Open);
    // Moving and rotating the actual shield changes its covered direction.
    {State g;uint64_t n=5000;for(int i=0;i<40;++i)Update(g,chest,true,true,Vec{1,0,0},++n,true,t);
     assert(Judge(g,{1.5f,0,0},n,t)==Verdict::Block&&Judge(g,ahead,n,t)==Verdict::Open);}
    assert(std::fabs(s.forward-.25f)<1e-4f&&std::fabs(s.up+.35f)<1e-4f&&std::fabs(s.side-.12f)<1e-4f); // trace inputs

    // The diagonal: lower right to upper left in 90 ms parries...
    Hold(s,swingFrom,true,now,200,t);
    Move(s,swingFrom,swingTo,true,now,90,t);
    const uint64_t snap=s.snapAt;
    assert(snap&&!s.snapUsed&&s.swingCos>.9f&&Judge(s,ahead,now+20,t)==Verdict::Parry);
    // ...ranged attackers are blocked, never parried...
    assert(Judge(s,{0,0,-8.f},now+20,t)==Verdict::Block);
    // ...one parry per swing, even while the same swing goes on...
    SpendParry(s,snap);
    Move(s,swingTo,swingTo+Vec{-.06f,.06f,0},true,now,20,t);
    assert(s.snapAt==snap&&Judge(s,ahead,now,t)==Verdict::Block);
    // ...and it expires after the window.
    Hold(s,swingFrom,true,now,300,t);Move(s,swingFrom,swingTo,true,now,90,t);
    assert(!s.snapUsed&&Judge(s,ahead,s.snapAt+t.parryMs+1,t)!=Verdict::Parry);

    // The wrong diagonal, straight up, straight across and the reverse never parry.
    const Vec wrong[][2]={{{-.20f,-.48f,-.30f},{.18f,-.06f,-.32f}},   // lower left -> upper right
                          {{-.05f,-.50f,-.30f},{-.05f,-.05f,-.30f}},  // straight up
                          {{.20f,-.30f,-.30f},{-.25f,-.30f,-.30f}},   // straight across
                          {swingTo,swingFrom}};                       // upper left -> lower right
    for(const auto& w:wrong){State g;uint64_t n=20000;Hold(g,w[0],true,n,200,t);Move(g,w[0],w[1],true,n,90,t);
        assert(Judge(g,ahead,n,t)!=Verdict::Parry);}
    // Too slow, or too short, is a block.
    {State g;uint64_t n=30000;Hold(g,swingFrom,true,n,200,t);Move(g,swingFrom,swingTo,true,n,600,t);assert(Judge(g,ahead,n,t)==Verdict::Block);}
    {State g;uint64_t n=31000;Hold(g,chest,true,n,200,t);Move(g,chest,chest+Vec{-.06f,.06f,0},true,n,60,t);assert(Judge(g,ahead,n,t)==Verdict::Block);}
    // A swing that ends out of guard (over the head) cannot parry.
    {State g;uint64_t n=32000;Hold(g,swingFrom,true,n,200,t);Move(g,swingFrom,Vec{-.30f,.45f,-.30f},true,n,90,t);
     assert(Judge(g,ahead,n,t)==Verdict::Open);}
    // A right-hand shield mirrors: lower left -> upper right.
    {State g;uint64_t n=33000;const Vec a{-.18f,-.48f,-.30f},b{.20f,-.06f,-.32f};
     Hold(g,a,false,n,200,t);Move(g,a,b,false,n,90,t);assert(Judge(g,ahead,n+10,t)==Verdict::Parry);}

    // Holding still never produces swings.
    {State g;uint64_t n=34000;Hold(g,chest,true,n,500,t);assert(g.snapUsed&&Judge(g,ahead,n,t)==Verdict::Block);}
    // An untracked hand cannot guard.
    {State g;uint64_t n=35000;Hold(g,chest,true,n,40,t);Update(g,chest,false,true,view,++n,true,t);assert(!g.raised);}

    // The parry is a swing while the attack is incoming: after the attacker's
    // attack began, before the hit (within parryMs)...
    {State g;uint64_t n=40000;Hold(g,swingFrom,true,n,300,t);
     const uint64_t attack=n;Hold(g,swingFrom,true,n,150,t);              // the attack winds up
     Move(g,swingFrom,swingTo,true,n,90,t);
     assert(Judge(g,ahead,n+30,t,attack)==Verdict::Parry);}
    // ...not with a swing from before that attack began...
    {State g;uint64_t n=42000;Hold(g,swingFrom,true,n,300,t);Move(g,swingFrom,swingTo,true,n,90,t);
     const uint64_t attack=n+40;assert(Judge(g,ahead,n+120,t,attack)==Verdict::Block);
     assert(Judge(g,ahead,n+120,t,0)==Verdict::Parry);}                    // (attack start unknown: window alone)
    // ...and never after the hit: a swing after a blocked hit is only logged, and spends nothing.
    {State g;uint64_t n=44000;Hold(g,swingFrom,true,n,300,t);
     assert(Judge(g,ahead,n,t,n-200)==Verdict::Block);NoteBlock(g,n);
     uint64_t late=0;assert(!SwingAfterBlock(g,n,t,late));
     Hold(g,swingFrom,true,n,90,t);Move(g,swingFrom,swingTo,true,n,90,t);
     assert(SwingAfterBlock(g,n,t,late)&&late>=90&&late<=200&&!g.snapUsed);
     assert(!SwingAfterBlock(g,n,t,late));}                                // once

    // Parry disabled (window 0) leaves blocking intact.
    Tuning off=t;off.parryMs=0;
    {State g;uint64_t n=36000;Hold(g,swingFrom,true,n,200,t);Move(g,swingFrom,swingTo,true,n,90,t);assert(Judge(g,ahead,n+10,off)==Verdict::Block);}

    // A brief unposed frame (snap turn / recenter epoch) keeps the guard;
    // a real tracking or posing loss expires it after staleMs.
    Hold(s,chest,true,now,40,t);
    Update(s,{NAN,NAN,NAN},true,true,view,now+50,false,t);
    assert(s.raised&&Judge(s,ahead,now+50,t)!=Verdict::Open);
    assert(Judge(s,ahead,now+t.staleMs+1,t)==Verdict::Open);

    // Report the actual rejection without changing the guard thresholds.
    {const struct {Vec hand;Rejection why;} cases[]={
        {hanging,Rejection::TooLow},{{-.1f,.4f,-.2f},Rejection::TooHigh},
        {{-.1f,-.3f,-1.3f},Rejection::TooFar},
        {wide,Rejection::OutsidePlacement},{{.6f,-.3f,-.2f},Rejection::OutsidePlacement},
        {{NAN,0,0},Rejection::InvalidHand}};
     for(const auto& c:cases){State g;Update(g,c.hand,true,true,view,1000,true,t);Rejection why;
         assert(Judge(g,ahead,1000,t,0,&why)==Verdict::Open&&why==c.why);}
     State g;Update(g,chest,true,true,view,1000,true,t);Rejection why;
     assert(Judge(g,ahead,1000,t,0,&why)==Verdict::Block&&why==Rejection::None);
     assert(Judge(g,{0,0,1},1000,t,0,&why)==Verdict::Open&&why==Rejection::OutsidePlacement);
     assert(Judge(g,ahead,1151,t,0,&why)==Verdict::Open&&why==Rejection::StalePose);
     Update(g,chest,false,true,view,1001,true,t);
     assert(Judge(g,ahead,1001,t,0,&why)==Verdict::Open&&why==Rejection::Untracked);}

    // Head yaw alone changes the head-relative hand, never the physical shield.
    {State g;Pose p=Surface(chest,view,50000);Update(g,p,true,true,p.at,true,t);
     assert(Judge(g,ahead,p.at,t)==Verdict::Block);
     for(float angle:{1.5707963f,3.1415926f,-1.5707963f}) {
         p.hand=framevr::math::Rotate(framevr::math::Yaw(angle),chest);p.at++;
         Update(g,p,true,true,p.at,true,t);assert(Judge(g,ahead,p.at,t)==Verdict::Block);
     }
     p.normal={1,0,0};p.at++;Update(g,p,true,true,p.at,true,t);
     Rejection why;assert(Judge(g,ahead,p.at,t,0,&why)==Verdict::Open&&why==Rejection::OutsideCoverage);
     p.normal={0,1,0};p.at++;Update(g,p,true,true,p.at,true,t);assert(Judge(g,ahead,p.at,t)==Verdict::Open);
     p.normal={0,0,1};p.at++;Update(g,p,true,true,p.at,true,t);assert(Judge(g,ahead,p.at,t)==Verdict::Block); // either face
     p.center.x=-.8f;p.at++;Update(g,p,true,true,p.at,true,t);
     assert(Judge(g,ahead,p.at,t,0,&why)==Verdict::Open&&why==Rejection::OutsidePlacement);
    }
    // Same sample must age even if the UI polls it repeatedly. Tracking loss
    // opens immediately even when no new rendered pose arrives.
    {State g;const Pose p=Surface(chest,view,60000);Update(g,p,true,true,p.at,true,t);
     Update(g,p,true,true,p.at+t.staleMs+1,true,t);assert(Judge(g,ahead,p.at+t.staleMs+1,t)==Verdict::Open);
     Update(g,p,true,true,p.at,true,t);Update(g,p,false,true,p.at+1,false,t);assert(!g.raised);
     auto invalid=p;invalid.normal={NAN,0,0};Update(g,invalid,true,true,p.at,true,t);assert(!g.raised);
     invalid=p;invalid.at++;Update(g,invalid,true,true,p.at,true,t);assert(!g.raised);
    }
    // Attachment conversion: large world offsets, calibration scale, mirrored
    // axes and character yaw preserve the same physical coverage in metres.
    for(float units:{2.f,4.f,8.f})for(float yaw:{0.f,1.5707963f,3.1415926f})for(float mirror:{-1.f,1.f}) {
        const auto q=framevr::math::Yaw(yaw);
        const Vec r=framevr::math::Rotate(q,{1,0,0}),u{0,1,0},b=framevr::math::Rotate(q,{0,0,1});
        constexpr float scale=.93f;
        float model[16]={r.x*scale,r.y*scale,r.z*scale,0,u.x*scale,u.y*scale,u.z*scale,0,b.x*scale,b.y*scale,b.z*scale,0,23000,50,-18000,1};
        const Vec eye{.1f,1.7f,-.05f}; // include calibrated camera/body offset
        const Vec center=eye+chest;
        const Vec w=framevr::math::Rotate(q,center)*(units*scale);
        float world[16]={r.x*mirror,r.y*mirror,r.z*mirror,0,0,1,0,0,b.x,b.y,b.z,0,23000+w.x,50+w.y,-18000+w.z,1};
        Pose p;assert(FromAttachment(world,model,eye*units,chest,units,70000,9,p));
        const Vec expected=framevr::math::Rotate(q,center);
        assert(std::sqrt(framevr::math::Dot(p.center-expected,p.center-expected))<.002f);
        assert(std::fabs(p.unitsPerMetre-units*scale)<.0001f);
        State g;Update(g,p,true,true,p.at,true,t);
        assert(Judge(g,framevr::math::Rotate(q,ahead+Vec{eye.x,0,eye.z}),p.at,t)==Verdict::Block);
        world[0]=world[1]=world[2]=0;assert(!FromAttachment(world,model,eye*units,chest,units,70000,9,p));
    }
    std::puts("Shield guard: posed surface direction/placement, independent gaze, calibrated attachment transforms, movement, edge-on/lowered rejection, parries and source-pose expiry passed");
}
