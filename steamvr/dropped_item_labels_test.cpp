#include "dropped_item_labels.h"
#include "hand_controls.h"
#include <array>
#include <cassert>
#include <cstdio>
using namespace framevr;
int main() {
    using namespace lootlabels;
    Display look;
    assert(!look.Active(1000)&&look.Visible(1000));
    look.Refresh(1,1000,true);assert(look.Active(1000)&&look.Visible(1000));
    assert(!look.Toggle()&&!look.Visible(1001));
    look.Refresh(1,1100,true);assert(!look.Visible(1200));
    assert(!look.Active(1400)&&look.Visible(1400)); // no stale override in desktop play
    look.Refresh(1,1500,true);assert(!look.Visible(1500)); // same character retains the choice
    look.Refresh(1,1600,false);assert(!look.Active(1600)&&look.Visible(1600));
    look.Refresh(2,1700,true);assert(look.Visible(1700)); // a new character starts visible
    look.Toggle();look.Clear();assert(look.Visible(1701));
    look.Refresh(0,1800,true);assert(!look.Active(1800)&&look.Visible(1800));
    assert(TextScale==1.5f&&BackingAlpha(.6f)==.85f&&BackingAlpha(.95f)==.95f);
    assert(BackingAlpha(0)==0&&BackingAlpha(-1)==-1);
    Shortcut combo;uint64_t now=1000,session=1,epoch=1;
    auto step=[&](bool click,float grip=1,bool allowed=true){now+=20;return combo.Step(session,epoch,now,allowed,click,grip);};
    assert(!step(true)&&combo.Consumed()); // a combo held on first sample cannot toggle
    assert(!step(false)&&!combo.Consumed());
    assert(combo.GripConsumed()&&!combo.GripConsumed(true)); // releasing stick first cannot replay grip
    assert(step(true)&&combo.Consumed());
    for(int i=0;i<5;++i)assert(!step(true)); // once per press
    assert(!step(true,0)&&combo.Consumed()); // release grip first: no wheel replay
    assert(!combo.GripConsumed());
    assert(!combo.Consumed(true)); // native menu sort retains its stick click
    assert(!step(false,0)&&!combo.Consumed());
    assert(!step(true,0)&&!combo.Consumed());
    assert(!step(true,1)); // adding a grip to an already-clicked stick is not a new combo
    step(false);assert(step(true));
    for(int event=0;event<5;++event) {
        step(false);
        if(event==0)++session;if(event==1)++epoch;if(event==2)now+=1000;
        if(event==3)combo.Suspend();
        assert(!step(true,1,event!=4));
        assert(!step(true,1)); // recovery cannot replay a held click
        step(false);assert(step(true));
    }
    step(false);assert(!step(true,NAN));step(false);assert(step(true));
    // A consumed press never opens the hand wheel; an ordinary click still does.
    skills::HandControls hands;skills::HandInput input;input.context=input.layout=1;input.active=true;
    for(int i=0;i<3;++i){input.time+=.02;hands.Step(input);}
    input.click[0]=true;input.click[0]&=!combo.Consumed();input.time+=.02;
    assert(hands.Step(input).wheel==-1);
    step(true,0);input.click[0]=!combo.Consumed();input.time+=.02;assert(hands.Step(input).wheel==-1);
    step(false,0);input.click[0]=false;input.time+=.02;hands.Step(input);
    step(true,0);input.click[0]=!combo.Consumed();input.time+=.02;assert(hands.Step(input).wheel==0);
    std::array<int32_t,4> box{100,150,120,28};
    const int32_t beforeText[]={100,20},afterText[]={150,30};
    assert(Resize(box.data(),beforeText,afterText,TextScale));
    assert((box==std::array<int32_t,4>{70,136,180,42})); // stable center/bottom, matching text and padding
    const auto keep=box;
    for(float bad:{NAN,INFINITY,0.f,3.f})assert(!Resize(box.data(),beforeText,afterText,bad)&&box==keep);
    const int32_t empty[]={0,0};assert(!Resize(box.data(),empty,afterText,TextScale)&&box==keep);
    box={INT32_MIN,150,120,28};const auto edge=box;
    assert(!Resize(box.data(),beforeText,afterText,TextScale)&&box==edge);
    float multiplier=0;
    {TextScaleScope scope(multiplier,TextScale);assert(multiplier==1.5f);
        {TextScaleScope nested(multiplier,1);assert(multiplier==1);}
        assert(multiplier==1.5f);}
    assert(multiplier==0); // other native text retains its original scale
    std::puts("Loot labels: contrast/geometry, scoped text scale, visibility expiry, combo consumption and lifecycle guards passed");
}
