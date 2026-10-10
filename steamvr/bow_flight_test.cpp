#include "bow_flight.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr::bowflight;
static void Check(bool ok,const char* what) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
static bool Near(float a,float b){return std::fabs(a-b)<.0002f;}
static Shot Release() {
    Shot s;s.origin={100,1.4f,100};s.direction={0,0,-1};s.player={100,0,100};
    s.unitsPerMetre=4;s.draw=1;s.session=11;s.epoch=7;s.sequence=1;s.at=1000;s.equipment=42;return s;
}
static Vec Point(const float* m,Vec p) {
    return {m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};
}
int main() {
    {
        const auto shot=Release();const Gate gate{true,11,7,1000,42};Once once;
        Check(Valid(shot)&&Allowed(shot,gate,1100),"fresh coherent release accepted");
        Check(once.Take(shot,gate,1100)&&!once.Take(shot,gate,1100),"one native fire per release sequence");
        auto s=shot;s.sequence=2;Check(once.Take(s,gate,1100),"new draw can shoot");
        Check(!Allowed(s,gate,1151)&&!Allowed(s,gate,999),"expired/future input cannot spend ammo");
        for(int i=0;i<5;++i) {
            auto g=gate;
            if(i==0)g.active=false;
            if(i==1)++g.session;
            if(i==2)++g.epoch;
            if(i==3)++g.equipment;
            if(i==4)g.at=800;
            Check(!Allowed(s,g,1100),"menu/tracking, session, recenter, equipment, stale gate cancel release");
        }
        s=shot;s.origin.y=100;Check(!Valid(s),"teleported release rejected");
        s=shot;s.direction={0,0,0};Check(!Valid(s),"invalid aim rejected");
        s=shot;s.origin.x=std::numeric_limits<float>::quiet_NaN();Check(!Valid(s),"nonfinite release rejected");
    }
    {
        const Vec bottom{0,.3f,0},top{0,1.6f,0};float t=0;
        Check(CapsuleEntry({-5,1,0},{5,1,0},bottom,top,.3f,t)&&Near(t,.47f),"fast arrow cannot tunnel through torso");
        Check(CapsuleEntry({0,4,0},{0,-4,0},bottom,top,.3f,t)&&Near(t,.2625f),"vertical arrow hits top cap");
        Check(CapsuleEntry({0,-4,0},{0,4,0},bottom,top,.3f,t)&&Near(t,.5f),"vertical arrow hits bottom cap");
        Check(!CapsuleEntry({-5,2,0},{5,2,0},bottom,top,.3f,t),"arrow above head misses despite same ground path");
        Check(!CapsuleEntry({-5,1,.4f},{5,1,.4f},bottom,top,.3f,t),"side clearance does not damage");
        Check(CapsuleEntry({0,1,0},{0,1,0},bottom,top,.3f,t)&&t==0,"inside start handled without divide by zero");
        Check(!CapsuleEntry({3,1,0},{3,1,0},bottom,top,.3f,t),"stationary arrow outside body misses");
        Check(CapsuleEntry({-2,1,-2},{2,1,2},bottom,top,.3f,t)&&t>.44f&&t<.46f,"oblique high-speed sweep");
        Check(CapsuleEntry({-1,1,0},{1,1,0},top,top,.7f,t),"degenerate capsule is a sphere");
        for(float speed:{20.f,40.f,100.f}) {
            const float step=speed*.04f;
            Check(CapsuleEntry({-step*.5f,1,0},{step*.5f,1,0},bottom,top,.12f,t),"small creature hit across full 25Hz step");
        }
    }
    {
        const Vec from{10.25f,1,10.25f},to{30.25f,1,10.25f};
        const auto wall=[](int x,int){return x==15;};
        const float stop=WallEntry(from,to,2,wall);
        Check(Near(stop,4.75f/20),"DDA catches one-cell wall in long segment");
        Check(WallEntry(from,to,2,[](int,int){return false;})==1,"clear segment reaches endpoint");
        Check(WallEntry(from,from,2,[](int,int){return false;})==1,"vertical trajectory has stable ground cell");
        Check(WallEntry(from,to,2,[](int x,int){return x>=12;})<.1f,"unloaded cells stop flight");
        Check(Near(WallEntry(from,{8.25f,1,8.25f},2,[](int x,int z){return x==9&&z==10;}),.125f),"corner touches both neighbouring cells");
        Check(Near(WallEntry(from,{20.25f,1,20.25f},2,[](int x,int z){return x==11&&z==10;}),.075f),"positive diagonal cannot slip through corner");
        Check(WallEntry({-1,1,1},to,2,wall)==0,"out-of-world release blocked");
        float body=0;
        Check(CapsuleEntry(from,to,{18,.2f,10.25f},{18,1.7f,10.25f},.3f,body)&&stop<body,"wall occludes body behind it");
    }
    {
        Check(Near(Speed(4608,1),20)&&Near(Speed(9216,1),40),"configured speed maps into physical metres per second");
        Check(Near(Speed(2304,1),10)&&Speed(0,1)==0,"native slow/stop velocity preserved");
        Check(Speed(9216,.3f)<Speed(9216,1)&&Speed(100000,1)==100,"draw scales speed with bounded maximum");
        Flight f;f.shot=Release();f.speed=40;f.updatedAt=1000;f.alive=true;
        Check(f.Remember(0)&&!f.Remember(0)&&f.Seen(0),"piercing cannot hit the same native id twice, including zero");
        for(uint32_t i=1;i<64;++i)Check(f.Remember(i),"bounded pierce ledger");
        Check(!f.Remember(64),"full ledger fails closed");
        Check(Near(f.TailAt(1020).z,99.2f)&&Near(f.TailAt(9000).z,98.4f),"render prediction capped to one game tick during pause/stall");
        f.alive=false;Check(Near(f.TailAt(9000).z,100),"ended arrow cannot extrapolate through hit");
        f.alive=true;f.distance=2;
        for(const Vec d:{Vec{0,0,-1},Vec{1,0,0},Vec{0,1,0},Vec{0,-1,0},Vec{.6f,0,.8f}}) {
            f.shot.direction=d;float m[16]{};ArrowMatrix(f,1000,m);
            const auto tail=Point(m,{0,0,1}),tip=Point(m,{0,0,-.98391676f});
            Check(Length(tail-f.TailAt(1000)*4)<.0002f,"mesh tail follows frozen physical release");
            Check(Length((tip-tail)-d*2.8f)<.0002f,"mesh axis agrees with flight direction at every pitch/yaw");
            for(const float v:m)Check(std::isfinite(v),"vertical aim produces finite visual transform");
        }
    }
    {
        // arrow.json: a unit root plus a child mesh translated +5 native Y
        // and scaled 1.25. The initializer visits both; either order must bind
        // the mesh so this authored transform is replaced, never inherited.
        constexpr uint32_t root=0x10002,mesh=0x10003,helper=0x10004;
        Flight f;f.shot=Release();float flight[16]{};ArrowMatrix(f,1000,flight);
        for(bool rootFirst:{false,true}) {
            VisualBinding binding;
            const auto visit=[&](uint32_t entity){return binding.Bind(entity,entity==mesh);};
            if(rootFirst)Check(!visit(root),"parent without release component cannot claim arrow");
            Check(visit(mesh),"visible release mesh claims arrow");
            Check(!visit(root)&&!visit(helper),"later root/helper cannot overwrite mesh binding");
            Check(visit(mesh),"repeat initialization of same mesh is stable");
            Check(!binding.Bind(helper,true),"second component cannot steal established mesh");
            // Apply the same runtime override to a small two-node hierarchy.
            // A root override would place the child above the release; a mesh
            // override must put the actual rendered tail at the shot origin.
            const auto renderTail=[&](const VisualBinding& owner) {
                const Vec localTail{0,5,1.25f};
                const Vec inherited=owner.Matches(root)?Point(flight,localTail):localTail;
                return owner.Matches(mesh)?Point(flight,{0,0,1}):inherited;
            };
            Check(Length(renderTail(binding)-f.shot.origin*4)<.0002f,"child height and scale do not displace released arrow");
            VisualBinding wrong;Check(wrong.Bind(root,true),"negative control can reproduce old parent binding");
            Check(renderTail(wrong).y-f.shot.origin.y*4>5,"old parent binding reproduces above-player origin");
            constexpr uint32_t recycled=mesh+(1u<<20);
            Check(!binding.Matches(recycled)&&!binding.Bind(recycled,true),"reused entity index cannot inherit flight transform");
            binding.Clear();
            Check(!binding.Bound()&&!binding.Matches(mesh)&&binding.Bind(recycled,true),"new arrow may bind new generation after cleanup");
        }
    }
    std::puts("Physical bow: release ownership, cancellation, swept contacts, walls, speed, visual-axis and mesh-binding tests passed");
}
