#include "melee_contact.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace framevr;using namespace framevr::melee;
void Check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
struct Sim {
    Engine engine;Sample sample;Sweep sweep;
    Sim(){sample.active=true;sample.epoch=1;sample.equipment=1;sample.unitsPerMetre=4;
        sample.hand.p={0,1,-.5f};sample.blade={{0,1,-.5f},{0,1,-1.2f},.04f};Step(8);}
    bool Step(int n=1){bool result=false;while(n--){sample.time+=.02;result=engine.Step(sample,sweep);}return result;}
    bool Move(Vec d){sample.hand.p=sample.hand.p+d;sample.blade.base=sample.blade.base+d;sample.blade.tip=sample.blade.tip+d;return Step();}
};
int main(){
    const Body body{{0,.3f,0},{0,1.5f,0},.25f};
    Check(Touches({{-.6f,.8f,-.3f},{-.6f,.8f,.3f},.04f},{{.6f,.8f,-.3f},{.6f,.8f,.3f},.04f},body),"fast blade crossing between endpoints hits");
    Check(!Touches({{-.6f,2.1f,-.3f},{-.6f,2.1f,.3f},.04f},{{.6f,2.1f,-.3f},{.6f,2.1f,.3f},.04f},body),"swing above head misses");
    Check(!Touches({{-.6f,.8f,1},{-.6f,.8f,2},.04f},{{.6f,.8f,1},{.6f,.8f,2},.04f},body),"swing beside creature misses");
    Check(Touches({{0,1,-1},{0,1,-.5f},.04f},{{0,1,-.5f},{0,1,.1f},.04f},body),"thrust tip contact");
    Check(Touches({{0,2,0},{0,2.6f,0},.04f},{{0,1,0},{0,1.6f,0},.04f},body),"vertical chop contact");
    Check(Touches({{0,1,-.7f},{0,1,-.7f},.08f},{{0,1,.1f},{0,1,.1f},.08f},body),"unarmed degenerate segment is a swept fist");
    Check(SegmentDistanceSquared({0,0,0},{1,0,0},{0,1,0},{1,1,0})==1,"parallel segment distance");
    {
        const Blade left{{0,1,0},{-.707107f,1,-.707107f},.02f},right{{0,1,0},{.707107f,1,-.707107f},.02f};
        const Body edge{{0,.95f,-1},{0,1.05f,-1},.04f},beyond{{0,.95f,-1.3f},{0,1.05f,-1.3f},.04f};
        Check(Touches(left,right,edge),"swept slash reaches outer arc, not just shortened endpoint chord");
        Check(!Touches(left,right,beyond),"arc sweep does not extend weapon reach");
    }
    {
        Sim s;int powered=0;uint64_t stroke=0;
        for(int i=0;i<12;++i)if(s.Move({.04f,0,0})){++powered;if(!stroke)stroke=s.sweep.stroke;Check(stroke==s.sweep.stroke,"one stroke through follow-through");}
        Check(powered>0,"ordinary free slash has energy without any button state");
        s.Step(10);Check(!s.sweep.stroke,"resting blade has no damaging sweep");
        bool backhand=false;for(int i=0;i<8;++i)if(s.Move({-.04f,0,0})){backhand=true;Check(s.sweep.stroke!=stroke,"new slash gets distinct identity");}
        Check(backhand,"free backhand works");
    }
    {
        Sim s;uint64_t stroke=0;for(int i=0;i<16;++i)if(s.Move({.04f,0,0}))stroke=s.sweep.stroke;
        bool reversed=false;for(int i=0;i<8;++i)if(s.Move({-.04f,0,0})){reversed=true;Check(s.sweep.stroke>stroke,"deliberate reversal rearms without a pause");}
        Check(reversed,"continuous alternating slashes work");
    }
    {
        Sim s;
        for(int i=0;i<30;++i){s.sample.blade.base.x+=.03f;s.sample.blade.tip.x+=.03f;Check(!s.Step(),"camera/locomotion moving weapon cannot supply swing power");}
        for(int i=0;i<100;++i)Check(!s.Move({i%2?.02f:-.02f,0,0}),"tiny repeated hand jitter cannot become a stroke");
        bool afterJitter=false;for(int i=0;i<8;++i)afterJitter|=s.Move({-.04f,0,0});
        Check(afterJitter,"sub-threshold jitter cannot poison the next deliberate slash's energy history");
        Check(!s.Move({1,0,0}),"tracking teleport cannot hit");
        for(int i=0;i<3;++i)Check(!s.Move({.04f,0,0}),"discontinuity discards immediate motion");
        bool recovered=false;for(int i=0;i<10;++i)recovered|=s.Move({-.04f,0,0});
        Check(recovered,"fresh continuous motion recovers from tracking jump without holding still");
    }
    {
        Sim s;for(int i=0;i<5;++i)s.Move({.04f,0,0});
        ++s.sample.epoch;Check(!s.Move({.04f,0,0}),"snap turn/recenter cancels old stroke");
        s.Step(8);++s.sample.equipment;Check(!s.Move({.04f,0,0}),"equipment cancels previous geometry");
        s.Step(8);s.sample.active=false;Check(!s.Move({.04f,0,0}),"menus and pose loss clear sweep");
        s.sample.active=true;Check(!s.Move({.04f,0,0}),"returning from menu primes anew");
        s.Step(8);s.sample.time+=.3;Check(!s.Move({.04f,0,0}),"hitch cannot replay stale stroke");
        s.sample.hand.p.x=std::numeric_limits<float>::quiet_NaN();Check(!s.Step(),"invalid pose rejected");
    }
    for(double dt:{1./36,1./72,1./120}) {
        Sim s;bool contact=false;
        for(int i=0;i<30;++i){s.sample.time+=dt;const Vec d{float(dt)*2,0,0};s.sample.hand.p=s.sample.hand.p+d;s.sample.blade.base=s.sample.blade.base+d;s.sample.blade.tip=s.sample.blade.tip+d;contact|=s.engine.Step(s.sample,s.sweep);}
        Check(contact,"frame-rate independent energy threshold");
    }
    {
        Sim s;bool powered=false;
        for(int i=1;i<10;++i){s.sample.hand.q=math::Yaw(i*.1f);s.sample.blade.tip=s.sample.blade.base+math::Rotate(s.sample.hand.q,{0,0,-.7f});powered|=s.Step();}
        Check(powered,"wrist-led slash supplies blade energy");
        s.Step(8);for(int i=0;i<8;++i){auto& q=s.sample.hand.q;q={-q.x,-q.y,-q.z,-q.w};Check(!s.Step(),"quaternion sign is not a new rotation");}
    }
    {
        Sim s;uint64_t first=0,last=0;int distinct=0;
        for(int half=0;half<4;++half)for(int i=1;i<=14;++i) {
            const float angle=half%2?1.4f-i*.1f:i*.1f;
            s.sample.hand.q=math::Yaw(angle);
            s.sample.blade.tip=s.sample.blade.base+math::Rotate(s.sample.hand.q,{0,0,-.7f});
            if(s.Step()&&s.sweep.stroke!=last){last=s.sweep.stroke;if(!first)first=last;++distinct;}
        }
        Check(first&&distinct==4,"four wrist-only alternating slashes are four strokes without pausing or translating the hand");
    }
    {
        Sim s;const auto onset=s.sample.blade;
        bool started=false;
        for(int i=0;i<5&&!started;++i)if(s.Move({.04f,0,0})) {
            started=true;Check(Length(s.sweep.before.base-onset.base)<.001f,"first powered contact includes initial acceleration path");
        }
        Check(started,"short deliberate swing starts");
    }
    // The owner's captured slashes reached 53 rad/s. The old upper speed
    // cutoff rejected the strongest part of a slash, then a tracking guard
    // required 80 ms below 0.45 m/s: 35 seconds of combat produced no sweeps.
    // Synthetic traces cover each swing plane without shipping pose captures.
    for(Vec axis:{Vec{1,0,0},Vec{0,1,0},Vec{0,0,1},math::Normalized(Vec{1,1,1})}) {
        Sim s;int strong=0;uint64_t last=0;int strokes=0;
        const Vec blade=std::fabs(axis.z)>.5f?Vec{.7f,0,0}:Vec{0,0,-.7f};
        for(int loop=0;loop<8;++loop)for(float a:{-1.4f,-.5f,.4f,1.3f,1.5f,.6f,-.3f,-1.2f}) {
            s.sample.hand.q={axis.x*std::sin(a/2),axis.y*std::sin(a/2),axis.z*std::sin(a/2),std::cos(a/2)};
            s.sample.blade.tip=s.sample.blade.base+math::Rotate(s.sample.hand.q,blade);
            if(s.Step()) {
                strong+=s.sweep.speed>18;
                if(s.sweep.stroke!=last){last=s.sweep.stroke;++strokes;}
            }
        }
        Check(strong>=12,"strong lateral, vertical and diagonal rotations remain damaging");
        Check(strokes>=4,"continuous strong swings rearm without a stationary pause");
    }
    {
        Sim s;for(int i=0;i<8;++i)s.Move({.04f,0,0});
        s.sample.time+=.25;Check(!s.Move({.04f,0,0}),"hitch itself never damages");
        for(int i=0;i<3;++i)Check(!s.Move({-.04f,0,0}),"hitch recovery drops old geometry");
        bool resumed=false;for(int i=0;i<12;++i)resumed|=s.Move({-.04f,0,0});
        Check(resumed,"continued swinging recovers after hitch without requiring a quiet hand");
    }
    {
        PoseHistory history;Sim s;Sample matched;
        history.Push(10,s.sample);s.sample.time+=.02;s.sample.hand.p.x=.1f;history.Push(11,s.sample);
        Check(history.Find(10,s.sample.time,matched)&&matched.hand.p.x==0,"rendered blade resolves its own earlier hand sample");
        Check(!history.Find(9,s.sample.time,matched),"unknown stamp cannot use latest hand as fallback");
        Check(!history.Find(10,s.sample.time+.11,matched),"expired input cannot replay a contact");
        ++s.sample.epoch;history.Push(12,s.sample);
        Check(!history.Find(10,s.sample.time,matched),"recenter invalidates earlier rendered pose");
        ++s.sample.equipment;history.Push(13,s.sample);
        Check(!history.Find(12,s.sample.time,matched),"weapon switch invalidates earlier rendered pose");
        s.sample.active=false;history.Push(14,s.sample);
        Check(!history.Find(13,s.sample.time,matched),"menu or trigger immediately disables older melee poses");
        history.Clear();Check(!history.Find(14,s.sample.time,matched),"tracking loss clears history");
        s.sample.active=true;history.Push(0,s.sample);
        Check(history.Find(0,s.sample.time,matched),"wrapped zero input stamp remains valid");
    }
    {
        // A quick slash ends between input update and the weapon pose. Pairing
        // its stale blade with the new velocity misses; when the blade catches
        // up the current hand is stationary. Timestamp pairing retains the hit.
        Engine mismatched,matched;PoseHistory history;Sample input;Sweep sw;
        input.active=true;input.epoch=1;input.equipment=1;input.unitsPerMetre=4;
        input.hand.p={-.16f,1,-.5f};input.blade={{-.16f,1,-.5f},{-.16f,1,-1.2f},.025f};
        const Body target{{0,.9f,-.7f},{0,1.1f,-.7f},.025f};
        Sample rendered=input;uint32_t renderStamp=1;history.Push(renderStamp,input);
        bool oldHit=false,newHit=false;
        for(uint32_t stamp=2;stamp<24;++stamp) {
            input.time+=.02;
            if(stamp==12){input.hand.p.x=0;input.blade.base.x=input.blade.tip.x=0;}
            history.Push(stamp,input);
            auto mixed=input;mixed.blade=rendered.blade;
            if(mismatched.Step(mixed,sw))oldHit|=Touches(sw.before,sw.after,target);
            Sample exact;
            Check(history.Find(renderStamp,input.time,exact),"one-frame pose history remains available");
            exact.blade=rendered.blade;
            if(matched.Step(exact,sw))newHit|=Touches(sw.before,sw.after,target);
            rendered=input;renderStamp=stamp;
        }
        Check(!oldHit,"reproduction: latest input with delayed blade loses the fast slash");
        Check(newHit,"matched blade and input retain the fast slash after hand stops");
    }
    HitSet hits;Check(hits.Insert(1,1,0),"unit zero is valid");Check(!hits.Insert(1,1,0),"one damage request per target per stroke");
    Check(hits.Insert(1,1,9),"same stroke can strike another actual contact");Check(hits.Insert(1,2,0),"next stroke can hit same creature");
    Check(hits.Insert(2,1,0),"new session clears old identities");
    std::puts("Melee contact: swept geometry, stroke energy, cancellation and per-target deduplication passed");
}
