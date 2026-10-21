#include "map_zoom.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr;
static void Check(bool b,const char* label){if(!b){std::fprintf(stderr,"FAIL: %s\n",label);std::exit(1);}}
struct Sim {
    mapzoom::Gesture g;mapzoom::Input in;mapzoom::Output out;int ticks=0,stops=0,starts=0,ends=0;
    Sim(){in.time=1;Step();}
    mapzoom::Output Step(int n=1){while(n--){in.time+=.02;out=g.Step(in);ticks+=out.tick==1;stops+=out.tick==2;starts+=out.started;ends+=out.stopped;}return out;}
};
// The map's center `degrees` off the head's forward, half a metre away.
static math::Vec At(float degrees){const float r=degrees*.0174532925f;return {.5f*std::sin(r),1.6f,-.5f*std::cos(r)};}
int main(){
    {
        const math::Vec head{0,1.6f,0};const math::Quat q{0,0,0,1};
        Check(mapzoom::Looking(head,q,At(0),false)&&mapzoom::Looking(head,q,At(20),false),"a map ahead is looked at");
        Check(!mapzoom::Looking(head,q,At(26),false),"past 22 degrees a glance does not start it");
        Check(mapzoom::Looking(head,q,At(26),true)&&mapzoom::Looking(head,q,At(29),true),"once looking, it holds to 30 degrees");
        Check(!mapzoom::Looking(head,q,At(32),true),"looking further away ends it");
        Check(!mapzoom::Looking(head,q,At(180),true),"a map behind is never looked at");
        Check(!mapzoom::Looking(head,q,head,false)&&!mapzoom::Looking(head,q,{NAN,0,0},true)&&!mapzoom::Looking(head,{NAN,0,0,1},At(0),true),
              "a missing pose is not looking");
        const math::Quat down{-.38268343f,0,0,.92387953f};   // head pitched 45 degrees down
        Check(mapzoom::Looking(head,down,{0,1.1f,-.5f},false)&&!mapzoom::Looking(head,q,{0,1.1f,-.5f},false),"the head's pitch counts, not just its heading");
    }
    {
        Sim s;Check(!s.out.owned&&s.out.zoom==1,"not looking: the stick is the game's and the map keeps its size");
        s.in.stick=1;Check(!s.Step(10).owned&&s.out.zoom==1&&!s.starts,"the stick alone never zooms");
        s.in.stick=0;s.in.looking=true;Check(s.Step().owned&&!s.out.zooming,"looking takes the stick's up / down, even at rest");
        Check(!s.out.save,"looking without moving the stick saves nothing");
        s.in.stick=1;s.Step(50);
        Check(s.out.zooming&&s.out.zoom>2.75f&&s.out.zoom<3,"up zooms in smoothly, 1.5 doublings a second");
        Check(s.starts==1&&s.ticks==7&&s.stops==0,"one start, one light tick per quarter crossed");
        s.Step(50);Check(s.out.zoom==mapzoom::Max&&s.stops==1,"zoom stops at the top with one firm tick");
        s.in.stick=-1;s.Step(200);Check(s.out.zoom==mapzoom::Min&&s.stops==2&&s.starts==1,"down returns exactly to the whole map");
        s.in.stick=.6f;s.Step(10);const float z=s.out.zoom;Check(z>1,"part-way stick zooms in");
        Check(!s.out.save,"no save while the stick is held");
        s.in.stick=.2f;Check(s.Step(10).zoom==z&&s.ends==1,"inside the dead zone the zoom holds and the gesture ends");
        Check(s.out.owned&&s.out.save,"the stick still belongs to the map, and the changed zoom asks to be saved");
        s.g.Submitted(s.in.time);Check(!s.Step().save&&s.out.zoom==z,"a submitted save is not repeated and the zoom stays while it is written");
        s.in.saved=z;Check(s.Step().zoom==z&&!s.out.save,"the saved value then leads");
        s.in.looking=false;s.in.stick=1;Check(!s.Step().owned&&s.out.zoom==z,"looking away gives the stick back the same frame");
        s.in.saved=2;Check(s.Step().zoom==2,"an edit to the stored zoom shows when not zooming");
    }
    {
        Sim s;s.in.looking=true;s.in.stick=-1;s.Step(10);
        Check(s.out.zoom==1&&s.out.zooming,"down at the whole map stays there");
        s.in.stick=1;s.Step(10);s.in.looking=false;
        Check(s.Step().stopped&&!s.out.zooming&&s.out.save,"looking away mid-zoom ends the gesture and saves");
        s.in.looking=true;s.Step();s.g.Reset();
        Check(s.Step().zooming&&s.out.started,"after a reset a held stick starts a new gesture");
    }
    {
        Sim s;s.in.looking=true;s.in.stick=1;s.Step(10);s.in.stick=0;s.Step();
        Check(s.out.zoom>1,"zoomed and let go");s.g.Submitted(s.in.time);
        s.in.time+=mapzoom::SaveWait;Check(s.Step().zoom==1,"a save never seen back gives way to the stored zoom");
    }
    {
        mapzoom::Gesture g;mapzoom::Input in;in.time=1;in.saved=NAN;Check(g.Step(in).zoom==1,"a malformed stored zoom shows the whole map");
        in.saved=9;Check(g.Step(in).zoom==mapzoom::Max,"an out-of-range stored zoom is clamped");
        in.looking=true;in.stick=NAN;in.time+=.02;Check(!g.Step(in).zooming&&g.Zoom()==mapzoom::Max,"a lost stick reading does nothing");
        Check(mapzoom::Clamp(.5f)==1&&mapzoom::Clamp(5)==4&&mapzoom::Clamp(2.5f)==2.5f&&mapzoom::Clamp(INFINITY)==1,"zoom clamps to 1x-4x");
        Check(mapzoom::NotchOf(1)==0&&mapzoom::NotchOf(1.26f)==1&&mapzoom::NotchOf(4)==12,"quarter notches");
    }
    {
        settings::Snapshot s;s.ready=true;s.sequence=7;
        const auto r=mapzoom::SaveRequest(s,2.5f);
        Check(r.changed==(settings::ChangeMask{1}<<settings::MapZoom)&&r.values[settings::MapZoom]==2.5f&&r.sequence==8,"a save changes only the zoom");
        Check(settings::Normalize(settings::MapZoom,2.5f)==2.5f&&settings::Normalize(settings::MapZoom,.5f)==1&&settings::Normalize(settings::MapZoom,NAN)==1,"stored zoom keeps any value in range");
        Check(!mapzoom::SaveRequest(s,1).changed,"an unchanged zoom writes nothing");
        s.ready=false;Check(!mapzoom::SaveRequest(s,2).sequence,"no configuration: nothing to save into");
        s.ready=true;s.result=settings::Result::Saving;Check(!mapzoom::SaveRequest(s,2).sequence,"another save in flight waits");
        Check(settings::Defaults()[settings::MapZoom]==1,"the map at its old size by default");
    }
    return 0;
}
