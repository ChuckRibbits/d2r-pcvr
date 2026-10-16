#include "bow_flight.h"
#include "gameplay.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr;
using namespace framevr::bowflight;
static void Check(bool ok,const char* what) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
static bool Near(float a,float b){return std::fabs(a-b)<.0005f;}
static Shot Release() {
    Shot s;s.origin={100,1.4f,100};s.direction={0,0,-1};s.player={100,0,100};
    s.unitsPerMetre=4;s.draw=1;s.session=11;s.epoch=7;s.sequence=1;s.at=1000;s.equipment=42;return s;
}
static Vec Point(const float* m,Vec p) {
    return {m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};
}
// Child world = local * parent (row vectors): identity orientation, scale 1.
static void Child(const float* parent,Vec offset,float out[16]) {
    for(int i=0;i<12;++i)out[i]=parent[i];
    const auto p=Point(parent,offset);out[12]=p.x;out[13]=p.y;out[14]=p.z;out[15]=1;
}
int main() {
    using weaponskills::Quiver;
    {
        // Skills.txt srvdofunc 0 arrow skills and their Missiles.txt ids only.
        for(int id:{6,7,11,16,21,27,31})Check(weaponskills::QuiverSkill(id)&&weaponskills::Bow(id),"single-missile arrow skill loads");
        for(int id:{0,12,22,26,36,97})Check(!weaponskills::QuiverSkill(id),"volleys, guided, basic and spells stay native");
        for(int id:{12,27,28,40,41,85,87})Check(weaponskills::QuiverMissile(id),"arrow skill missile");
        for(int id:{-1,0,1,31,59,213})Check(!weaponskills::QuiverMissile(id),"basic/bolt/spell/volley missile rejected");
    }
    {
        Quiver q;
        auto r=q.Step(5,3,7,-1,12,true);
        Check(r.consumed&&r.change==Quiver::Change::Loaded&&q.Current()==weaponskills::Loaded{7,-1,12},"press loads Fire Arrow");
        for(int i=0;i<20;++i){r=q.Step(5,3,7,-1,12,true);Check(r.consumed&&r.change==Quiver::Change::None,"held press toggles once and never casts");}
        r=q.Step(5,-1,-1,-1,-1,true);Check(!r.consumed&&q.Current().skill==7,"release keeps the arrow loaded");
        r=q.Step(5,3,7,-1,12,true);Check(r.consumed&&r.change==Quiver::Change::Unloaded&&!q.Current().Active(),"same button again: basic arrows");
        q.Step(5,-1,-1,-1,-1,true);
        q.Step(5,3,7,-1,12,true);q.Step(5,-1,-1,-1,-1,true);
        r=q.Step(5,4,11,-1,40,true);Check(r.change==Quiver::Change::Loaded&&q.Current().skill==11,"other skill swaps directly");
        q.Step(5,-1,-1,-1,-1,true);
        r=q.Step(5,3,7,2,12,true);Check(r.change==Quiver::Change::Loaded&&q.Current().owner==2,"item-granted copy is a different arrow");
        q.Step(5,-1,-1,-1,-1,true);
        auto fresh=[&](int slot,int skill,int missile,bool eligible){q.Step(5,-1,-1,-1,-1,true);return q.Step(5,slot,skill,-1,missile,eligible);};
        r=fresh(2,36,58,true);Check(!r.consumed&&q.Current().skill==7,"non-arrow skill casts natively, quiver kept");
        r=fresh(2,12,-1,true);Check(!r.consumed,"Multiple Shot keeps its native button");
        r=fresh(3,7,59,true);Check(!r.consumed,"unexpected missile id is not loaded");
        r=fresh(3,11,40,false);Check(!r.consumed&&q.Current().skill==7,"without a ready bow the button casts natively");
        r=q.Step(5,3,11,-1,40,true);Check(!r.consumed&&q.Current().skill==7,"a native press stays native when the bow becomes ready");
        r=fresh(3,11,40,true);Check(r.consumed&&q.Current().skill==11,"next press loads");
        for(int i=0;i<3;++i){r=q.Step(5,3,11,-1,40,false);Check(r.consumed&&r.change==Quiver::Change::None&&q.Current().skill==11,"stale frame mid-press: no native cast, no second toggle");}
        r=q.Step(6,-1,-1,-1,-1,true);Check(!q.Current().Active(),"character/session change empties the quiver");
        r=q.Step(6,3,7,-1,12,true);Check(r.consumed&&q.Current().skill==7,"new character starts empty and loads normally");
    }
    {
        const Gate gate{true,11,7,1000,42};
        auto s=Release();s.skill=7;s.missile=12;
        Check(SkillArrow(s)&&MissileId(s)==12&&Valid(s)&&Allowed(s,gate,1100),"quiver release is a valid arrow");
        Check(ProjectileLength(s)==ArrowLength,"keeps the arrow's length");
        Check(Near(FlightSpeed(s,4608),FlightSpeed(Release(),4608)),"keeps the basic arrow's draw speed");
        auto low=s;low.draw=.2f;Check(FlightSpeed(low,4608)<FlightSpeed(s,4608),"weak draw is slower for skill arrows too");
        auto bad=s;bad.missile=59;Check(!Valid(bad),"skill/missile mismatch rejected");
        bad=s;bad.skill=12;bad.missile=213;Check(!Valid(bad),"Multiple Shot cannot use the single-arrow path");
        bad=s;bad.missile=-1;Check(!Valid(bad),"missing missile rejected");
        Check(!SkillArrow(Release())&&MissileId(Release())==0,"basic arrow unchanged");
        auto spell=Release();spell.kind=Kind::Spell;spell.skill=36;spell.missile=58;
        Check(!SkillArrow(spell),"hand spells are not quiver arrows");
    }
    {
        // The root-bound model must land exactly where a basic arrow mesh is.
        for(int missile:{12,27,40,41,85,87})for(Vec d:{Vec{0,0,-1},Vec{.6f,.3f,-.74f},Vec{-.2f,-.5f,.84f},Vec{0,.97f,.24f}}) {
            auto s=Release();s.skill=7;s.missile=missile;s.direction=math::Normalized(d);
            Flight f;f.shot=s;f.distance=3;f.alive=true;f.updatedAt=10;
            float arrow[16],root[16],model[16];
            ArrowMatrix(f,10,arrow);QuiverMatrix(f,10,root);
            Child(root,{0,QuiverModelHeight,QuiverModelAhead(missile)},model);
            for(int i=0;i<16;++i)Check(Near(model[i],arrow[i]),"quiver model matches basic arrow placement");
            for(int i=0;i<12;++i)Check(Near(root[i],arrow[i]),"root keeps arrow orientation and length scale");
        }
        Check(QuiverModelAhead(12)<0&&QuiverModelAhead(40)==0&&QuiverModelAhead(27)==0,"fire model sits behind its particles");
    }
    {
        auto pose=Release();float basic[16],held[16],quiver[16],hidden[16];
        PreviewMatrix(pose,true,basic);
        Flight f;f.shot=pose;ArrowMatrix(f,0,held);
        for(int i=0;i<16;++i)Check(Near(basic[i],held[i]),"basic nocked arrow unchanged");
        pose.skill=7;pose.missile=12;
        PreviewMatrix(pose,true,quiver,0);
        for(int i=0;i<16;++i)Check(Near(quiver[i],held[i]),"a basic held client keeps the basic matrix during a swap");
        PreviewMatrix(pose,true,quiver,12);f.shot=pose;QuiverMatrix(f,0,held);
        for(int i=0;i<16;++i)Check(Near(quiver[i],held[i]),"nocked Fire Arrow uses the root placement");
        PreviewMatrix(pose,false,hidden,12);
        Check(std::fabs(hidden[0])<std::fabs(quiver[0])*.01f+1e-6f&&Near(hidden[12],quiver[12]),"hidden quiver preview collapses in place");
    }
    {
        // Headset regression: a nocked Magic Arrow (27) was checked against
        // the basic id 0, refused and dropped alive, so each retry flew off
        // the bow as a native missile. Every unkept live client is destroyed.
        const uint32_t magic[3]{3,27,900},basic[3]{3,0,901},monster[3]{1,27,902};
        Check(CreatedClient(true,magic,27,true)==ClientUse::Keep,"requested quiver preview kept");
        Check(CreatedClient(true,magic,0,true)==ClientUse::Destroy,"unexpected missile destroyed, not leaked");
        Check(CreatedClient(true,basic,27,true)==ClientUse::Destroy,"basic client for a quiver request destroyed");
        Check(CreatedClient(true,magic,27,false)==ClientUse::Destroy,"stale/unwanted preview destroyed");
        Check(CreatedClient(true,basic,0,true)==ClientUse::Keep,"basic nocked arrow unchanged");
        Check(CreatedClient(false,magic,27,true)==ClientUse::Unreadable,"unreadable result is never killed");
        Check(CreatedClient(true,monster,27,true)==ClientUse::Unreadable,"non-missile unit is never killed");
    }
    {
        // With a quiver arrow the engine's bow slot is cleared: a full draw
        // is a physical release, never the native skill button pulse.
        gameplay::Engine engine;gameplay::Input in;gameplay::Output out;
        in.active=true;in.tracked[0]=in.tracked[1]=in.aimTracked=true;in.weapon=gameplay::Weapon::Bow;in.equipment=1;
        in.handBindings=true;in.bowSlot=-1;in.bowSkill=-1;in.skillContext=7;
        in.head.p={0,1.65f,0};in.hand[0].p={0,1.3f,-.4f};in.hand[1].p={0,1.3f,-.5f};in.aim=in.hand[0];
        auto step=[&](int n=1){while(n--){in.time+=.02;out=engine.Step(in);}};
        step(2);
        in.trigger=1;step();Check(out.bowDrawing,"nocks from the string");
        for(int i=0;i<10;++i){in.hand[0].p.z+=.04f;step();}
        Check(out.bowCharged,"draw charges");
        in.trigger=0;step();
        Check(out.bowRelease&&!out.nativeBowRelease&&out.skill<0,"quiver release is physical");
    }
    std::puts("quiver tests passed");
}
