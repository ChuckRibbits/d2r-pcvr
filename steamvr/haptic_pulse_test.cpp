#include "haptic_pulse.h"
#include <cstdio>
#include <cstdlib>
#include <atomic>
using namespace std::chrono_literals;
using namespace framevr::haptics;
static void Check(bool value,const char* message){if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main() {
    Pulses pulses;
    auto pulse=[&](int hand,auto duration,bool accept=true){return pulses.Pulse(hand,Category::Melee,duration,.95f,[&](float amplitude){Check(amplitude==.95f,"default strength preserves native amplitude");return accept;});};
    std::mutex mutex;std::condition_variable changed;std::array<int,2> stops{};
    auto callback=[&](int hand){std::lock_guard lock(mutex);++stops[hand];changed.notify_all();};
    auto wait=[&](int hand,int count,auto timeout){std::unique_lock lock(mutex);return changed.wait_for(lock,timeout,[&]{return stops[hand]>=count;});};
    Check(pulses.Start(callback),"start timer");
    Check(pulse(0,10ms),"send short impact");
    Check(wait(0,1,2s),"single impact stops even when no render/input update runs");
    Check(!wait(0,2,30ms),"one impact expires only once");
    Check(pulse(0,40ms),"old pulse");
    Check(pulse(0,200ms),"new pulse replaces deadline");
    Check(!wait(0,2,80ms),"old timeout cannot stop replacement pulse early");
    Check(pulse(1,10ms),"other hand impact");
    Check(wait(1,1,2s)&&wait(0,2,2s),"hands expire independently");
    Check(!pulse(0,10ms,false),"failed apply is not scheduled");
    Check(!wait(0,3,30ms),"failed pulse does not trigger later stop");
    pulse(0,5s);pulse(1,5s);
    pulses.Cancel();Check(wait(0,3,0ms)&&wait(1,2,0ms),"focus loss cancels both hands immediately");
    pulse(0,5s);pulses.Stop();
    Check(wait(0,4,0ms),"shutdown stops active haptic before session destruction");
    Check(!pulse(0,10ms),"no calls after shutdown");
    Check(pulses.Start(callback)&&pulse(1,10ms),"new session can restart");
    Check(wait(1,3,2s),"restarted timer expires");pulses.Stop();
    {
        Pulses tuned;Strengths levels;std::array<std::atomic<int>,2> canceled{};
        Check(tuned.Start([&](int h){++canceled[h];}),"start strength fixture");
        auto send=[&](int h,Category category,float amplitude,float expected,const Strengths* preview=nullptr){
            bool called=false;
            const bool sent=tuned.Pulse(h,category,5s,amplitude,[&](float scaled){called=true;Check(std::fabs(scaled-expected)<.00001f,"runtime receives scaled amplitude");return true;},preview);
            Check(sent==(expected>0)&&called==sent,"Off suppresses the actual runtime call");
        };
        for(int h=0;h<2;++h)for(int c=0;c<CategoryCount;++c)send(h,Category(c),.4f,.4f);
        levels.master=50;levels.category[int(Category::BowDraw)]=50;tuned.Configure(levels);
        send(0,Category::BowDraw,.4f,.1f);send(1,Category::Melee,.4f,.2f);
        levels.master=200;levels.category[int(Category::Melee)]=200;tuned.Configure(levels);
        send(0,Category::Melee,.95f,1);send(1,Category::BowDraw,.4f,.4f);
        levels.category[int(Category::BowDraw)]=0;tuned.Configure(levels);
        Check(canceled[0]==0&&canceled[1]==1,"muting bow draw stops only that category");
        send(1,Category::BowDraw,.4f,0);send(0,Category::Melee,.95f,1);
        levels.master=0;tuned.Configure(levels);
        Check(canceled[0]==1&&canceled[1]==1,"master Off stops the remaining active hand");
        for(int h=0;h<2;++h)for(int c=0;c<CategoryCount;++c)send(h,Category(c),.5f,0);
        Strengths preview;preview.master=50;preview.category[int(Category::Spell)]=150;
        send(0,Category::Spell,.4f,.3f,&preview);send(1,Category::Spell,.4f,.3f,&preview);
        tuned.Configure(levels); // unchanged saved Off must not cancel a deliberate unsaved preview
        Check(canceled[0]==1&&canceled[1]==1,"preview can test re-enabling muted feedback");
        send(0,Category::Spell,.4f,0); // preview never changes the saved strength
        tuned.Cancel();Check(canceled[0]==2&&canceled[1]==2,"closing settings cancels both previews");
        levels={};levels.master=NAN;tuned.Configure(levels);send(0,Category::Melee,.4f,.4f);
        send(0,Category::Melee,NAN,0);send(0,Category::Melee,-1,0);send(0,Category::Melee,2,1);
        tuned.Stop();
    }
    Check(MotionCategory(1,true,false)==Category::BowDraw&&MotionCategory(2,true,false)==Category::BowRelease,"bow draw and release have independent categories");
    Check(MotionCategory(4,false,false)==Category::BowDraw&&MotionCategory(5,true,false)==Category::BowDraw,"string proximity and full draw use draw strength");
    Check(MotionCategory(3,true,false)==Category::Interaction&&MotionCategory(1,false,false)==Category::Interaction,"potions and selection do not inherit equipped bow strength");
    Check(MotionCategory(2,false,true)==Category::BowRelease&&MotionCategory(6,false,false)==Category::Melee,"throw release and melee readiness route correctly");
    {
        CastEvents casts;casts.Publish(0,1,1000);casts.Publish(1,1,1000);
        Check(casts.Take(0,1,1001,true)&&!casts.Take(0,1,1001,true)&&casts.Take(1,1,1001,true),"each hand's successful cast is consumed once");
        casts.Publish(0,1,1100);Check(!casts.Take(0,2,1101,true),"another character cannot inherit spell feedback");
        casts.Publish(0,1,1200);Check(!casts.Take(0,1,1350,true),"stale spell pulse expires");
        casts.Publish(0,1,1400);Check(!casts.Take(0,1,1401,false)&&!casts.Take(0,1,1402,true),"menu or inactive frame consumes feedback without replay");
        casts.Publish(0,1,1500);casts.Clear();Check(!casts.Take(0,1,1501,true),"focus loss clears pending spells");
        casts.Publish(-1,1,1600);casts.Publish(2,1,1600);casts.Publish(0,0,1600);
        Check(!casts.Take(0,1,1601,true)&&!casts.Take(2,1,1601,true),"invalid hand or session never produces feedback");
    }
    std::puts("Haptics: master/categories, runtime amplitude, mute/cancel, preview isolation, cast events, expiry and lifecycle passed");
}
