#include "map_zoom.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr;
static void Check(bool b,const char* label){if(!b){std::fprintf(stderr,"FAIL: %s\n",label);std::exit(1);}}
struct Sim {
    mapzoom::Gesture g;mapzoom::Input in;mapzoom::Output out;int ticks=0,stops=0;
    Sim(){in.time=1;in.enabled=true;in.hand=1;Step();}
    mapzoom::Output Step(int n=1){while(n--){in.time+=.02;out=g.Step(in);ticks+=out.tick==1;stops+=out.tick==2;}return out;}
};
int main(){
    {
        Sim s;Check(!s.out.engaged&&s.out.zoom==1,"no grip: the stick walks and the map keeps its size");
        s.in.stick[1]=1;Check(!s.Step(10).engaged&&s.out.zoom==1,"stick alone never zooms");
        s.in.stick[1]=0;s.in.grip[1]=1;Check(s.Step().engaged&&s.out.hand==1,"grip pressed while the map shows takes that hand's stick");
        Check(!s.out.save,"holding grip without moving the stick saves nothing");
        s.in.stick[1]=1;s.Step(50);
        Check(s.out.engaged&&s.out.zoom>2.75f&&s.out.zoom<3,"forward zooms in smoothly, 1.5 doublings a second");
        Check(s.ticks==7&&s.stops==0,"one light tick per quarter crossed");
        s.Step(50);Check(s.out.zoom==mapzoom::Max&&s.stops==1,"zoom stops at the top with one firm tick");
        s.in.stick[1]=-1;s.Step(200);Check(s.out.zoom==mapzoom::Min&&s.stops==2,"pulling back returns exactly to the whole map");
        s.in.stick[1]=.6f;s.Step(10);const float z=s.out.zoom;Check(z>1,"part-way stick zooms in");
        s.in.stick[1]=.2f;Check(s.Step(10).zoom==z,"inside the dead zone the zoom holds");
        s.in.grip[1]=0;s.in.stick[1]=1;
        Check(!s.Step().engaged&&s.out.hand==-1&&s.out.zoom==z,"releasing grip gives the stick back the same frame");
        Check(s.out.save,"a changed zoom asks to be saved once the grip opens");
        s.g.Submitted(s.in.time);Check(!s.Step().save&&s.out.zoom==z,"a submitted save is not repeated and the zoom stays while it is written");
        s.in.saved=z;Check(s.Step().zoom==z&&!s.out.save,"the saved value then leads");
        s.in.saved=2;Check(s.Step().zoom==2,"an edit to the stored zoom shows when not zooming");
    }
    {
        Sim s;s.in.stick[1]=1;s.in.grip[1]=1;s.Step();
        Check(s.out.engaged&&s.Step(20).zoom==1,"a stick already pushed when grip closes waits for the centre");
        s.in.stick[1]=0;s.Step();s.in.stick[1]=1;Check(s.Step(5).zoom>1,"after centring it zooms");
    }
    {
        Sim s;s.in.hand=-1;s.in.grip[1]=1;s.Step();
        s.in.hand=1;s.in.stick[1]=1;Check(!s.Step(10).engaged&&s.out.zoom==1,"a grip held before the map showed is not a zoom");
        s.in.grip[1]=0;s.Step();s.in.busy=true;s.in.grip[1]=1;Check(!s.Step().engaged,"a busy hand (bottle, casting) never zooms");
        s.in.busy=false;s.in.grip[1]=0;s.Step();s.in.enabled=false;s.in.grip[1]=1;Check(!s.Step().engaged,"the VR setting turns it off");
    }
    {
        Sim s;s.in.grip[1]=1;s.Step();s.in.stick[1]=1;s.Step(10);
        s.in.hand=-1;Check(!s.Step().engaged,"the map going away ends the zoom at once");
        s.in.hand=1;Check(!s.Step(5).engaged,"and it needs a fresh grip");
        s.in.grip[1]=0;s.Step();s.in.hand=0;s.in.grip[0]=1;s.in.stick[0]=0;s.Step();s.in.stick[0]=1;
        Check(s.Step(5).engaged&&s.out.hand==0&&s.out.zoom>1,"a right-hand wheel's map zooms with the right grip and stick");
        s.in.grip[1]=1;s.in.stick[1]=-1;Check(s.Step().hand==0,"the other hand's grip and stick stay its own");
        s.in.hand=1;Check(!s.Step().engaged,"the map moving to the other hand ends the zoom");
    }
    {
        Sim s;s.in.grip[1]=1;s.Step();s.in.stick[1]=1;s.Step(10);s.in.grip[1]=0;s.Step();
        Check(s.out.zoom>1,"zoomed and released");s.g.Submitted(s.in.time);
        s.in.time+=mapzoom::SaveWait;Check(s.Step().zoom==1,"a save never seen back gives way to the stored zoom");
        s.in.grip[1]=1;s.Step();s.g.Reset();s.in.stick[1]=1;Check(!s.Step(5).engaged,"a reset needs a fresh grip");
        s.in.grip[1]=0;s.Step();s.in.grip[1]=1;Check(s.Step().engaged,"released and pressed again, it zooms");
    }
    {
        mapzoom::Gesture g;mapzoom::Input in;in.time=1;in.saved=NAN;Check(g.Step(in).zoom==1,"a malformed stored zoom shows the whole map");
        in.saved=9;Check(g.Step(in).zoom==mapzoom::Max,"an out-of-range stored zoom is clamped");
        in.enabled=true;in.hand=1;in.grip[1]=1;g.Step(in);in.stick[1]=NAN;in.time+=.02;Check(g.Step(in).zoom==mapzoom::Max,"a lost stick reading does nothing");
    }
    {
        settings::Snapshot s;s.ready=true;s.sequence=7;
        const auto r=mapzoom::SaveRequest(s,2.5f);
        Check(r.changed==(settings::ChangeMask{1}<<settings::MapZoom)&&r.values[settings::MapZoom]==2.5f&&r.sequence==8,"a save changes only the zoom");
        Check(settings::Normalize(settings::MapZoom,2.5f)==2.5f&&settings::Normalize(settings::MapZoom,.5f)==1&&settings::Normalize(settings::MapZoom,NAN)==1,"stored zoom keeps any value in range");
        Check(!mapzoom::SaveRequest(s,1).changed,"an unchanged zoom writes nothing");
        s.ready=false;Check(!mapzoom::SaveRequest(s,2).sequence,"no configuration: nothing to save into");
        s.ready=true;s.result=settings::Result::Saving;Check(!mapzoom::SaveRequest(s,2).sequence,"another save in flight waits");
        Check(settings::Defaults()[settings::MapGripZoom]==1&&settings::Defaults()[settings::MapZoom]==1,"grip zoom on, map at its old size by default");
    }
    return 0;
}
