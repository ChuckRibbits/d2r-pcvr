#include "spell_cast.h"
#include "bow_flight.h"
#include "hand_controls.h"
#include <cassert>
#include <cstdio>
using namespace framevr;
struct Sim {
    skills::HandControls controls;
    skills::HandInput input;skills::HandOutput output;
    Sim(){input.context=input.layout=1;input.active=true;input.independentSlots=(1u<<skills::TriggerSlot(0))|(1u<<skills::TriggerSlot(1));Step(3);}
    void Step(int n=1){while(n--){input.time+=.02;output=controls.Step(input);}}
};
int main() {
    for(int id:{36,39,45,47})assert(spellcast::Candidate(1,id,0,0,56,0));
    assert(!spellcast::Candidate(0,47,0,0,56,0));
    assert(!spellcast::Candidate(1,47,1,0,56,0));
    assert(!spellcast::Candidate(1,47,0,8,56,0));
    assert(!spellcast::Candidate(1,47,0,0,56,2));
    assert(!spellcast::Supported(219)&&!spellcast::Supported(53)&&!spellcast::Supported(64));
    constexpr int breaks[]={0,9,20,37,63,105,200};
    for(unsigned i=0;i<7;++i){
        assert(spellcast::CastFrames(breaks[i])==13-i);
        if(i)assert(spellcast::CastFrames(breaks[i]-1)==14-i);
    }
    assert(spellcast::CastFrames(-1)==13&&spellcast::CastFrames(100000)==7);
    spellcast::Cadence cadence;
    assert(cadence.Ready(1,0,0)&&cadence.Ready(1,1,0));
    cadence.Fired(1,0,100,13);
    assert(!cadence.Ready(1,0,112)&&cadence.Ready(1,1,100));
    cadence.Fired(1,1,100,8);
    assert(!cadence.Ready(1,1,107)&&cadence.Ready(1,1,108)&&cadence.Ready(1,0,113));
    assert(cadence.Ready(2,0,1));
    cadence.Fired(2,0,UINT32_MAX-5,13);
    assert(!cadence.Ready(2,0,6)&&cadence.Ready(2,0,7));
    {
        Sim s;s.input.trigger[0]=s.input.trigger[1]=1;s.Step();
        assert(s.output.slot==-1&&s.output.spells[0]==skills::TriggerSlot(0)&&s.output.spells[1]==skills::TriggerSlot(1));
        const auto left=s.output.activation[1],right=s.output.activation[0];
        s.input.trigger[0]=0;s.Step();assert(s.output.spells[0]<0&&s.output.activation[1]==left);
        s.input.trigger[0]=1;s.Step();assert(s.output.activation[0]!=right&&s.output.activation[1]==left);
        s.input.click[1]=true;s.Step();assert(s.output.wheel==1&&s.output.spells[0]<0&&s.output.spells[1]<0);
    }
    for(int event=0;event<6;++event){
        Sim s;s.input.trigger[0]=s.input.trigger[1]=1;s.Step();
        if(event==0)s.input.active=false;if(event==1)++s.input.context;if(event==2)++s.input.layout;
        if(event==3)s.input.occupied=true;if(event==4)s.input.time+=1;if(event==5)s.input.independentSlots=0;
        s.Step();assert(s.output.spells[0]<0&&s.output.spells[1]<0);
        s.input.active=true;s.input.occupied=false;s.Step(3);assert(s.output.spells[0]<0&&s.output.spells[1]<0&&s.output.slot<0);
    }
    {
        Sim s;s.input.face[1][0]=true;s.Step();assert(s.output.slot>=0);
        s.input.trigger[0]=1;s.Step();assert(s.output.spells[0]<0);
        s.input.face[1][0]=false;s.Step(3);assert(s.output.spells[0]<0); // no delayed cast
    }
    bowflight::Shot shot;shot.kind=bowflight::Kind::Spell;shot.skill=47;shot.slot=0;shot.hand=0;
    shot.session=shot.sequence=1;shot.unitsPerMetre=4;shot.origin={0,1.6f,0};shot.direction={0,1,0};
    assert(bowflight::Valid(shot));
    bowflight::Gate gate{true,1,0,0,0,false};
    assert(bowflight::Allowed(shot,gate,0));
    auto arrow=shot;arrow.kind=bowflight::Kind::Arrow;
    assert(!bowflight::Allowed(arrow,gate,0)); // widening spell eligibility must not release a canceled arrow
    const auto tip=bowflight::Tip(shot,0);assert(tip.y==shot.origin.y); // no arrow-length offset
    bowflight::Flight flight;flight.shot=shot;flight.distance=3;
    float matrix[16]={2,0,0,0,0,3,0,0,0,0,4,0,0,0,0,1};
    bowflight::SpellMatrix(flight,0,matrix);
    for(int i=0;i<3;++i)assert(std::fabs(bowflight::Length({matrix[i*4],matrix[i*4+1],matrix[i*4+2]})-(i+2))<.001f);
    const math::Vec emitter={matrix[12]+4*matrix[4]+2.3f*matrix[8],matrix[13]+4*matrix[5]+2.3f*matrix[9],matrix[14]+4*matrix[6]+2.3f*matrix[10]};
    assert(std::fabs(emitter.y-18.4f)<.001f&&std::fabs(emitter.x)<.001f&&std::fabs(emitter.z)<.001f);
    for(int skill:{36,39,45,47})for(int yaw=0;yaw<360;yaw+=5)for(int pitch:{-90,-40,0,40,90}) {
        const float y=yaw*.01745329252f,p=pitch*.01745329252f;
        flight.shot.skill=skill;flight.shot.direction={std::cos(p)*std::sin(y),std::sin(p),std::cos(p)*std::cos(y)};
        float m[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};bowflight::SpellMatrix(flight,0,m);
        const float z=skill==47?2.3f:0;
        const math::Vec visible={m[12]+4*m[4]+z*m[8],m[13]+4*m[5]+z*m[9],m[14]+4*m[6]+z*m[10]};
        assert(bowflight::Length(visible-bowflight::Tip(flight.shot,3)*shot.unitsPerMetre)<.0001f);
        assert(math::Dot({m[8],m[9],m[10]},flight.shot.direction)>.9999f);
    }
    assert(std::fabs(bowflight::FlightSpeed(shot,4096)-12.5f)<.001f);
    const auto frozen=bowflight::Tip(flight.shot,3);shot.direction={1,0,0};
    assert(bowflight::Tip(flight.shot,3).y==frozen.y);
    bowflight::VisualBinding binding;
    assert(!binding.BindRoot(14,true)&&binding.BindRoot(12,false)&&!binding.BindRoot(13,false));
    puts("Hand spells: dual ownership, cancellation, frozen 3D origins, cast cadence and eligibility passed");
}
