#include "dropped_bottles.h"
#include <cassert>
#include <cstdio>
#include <limits>

using namespace framevr;
using namespace framevr::bottles;
using math::Vec;
namespace {
constexpr float Upm=3.3f;
Release At(Vec p,Vec v={},float floor=0) {
    Release r;r.session=7;r.time=0;r.p=p;r.v=v;r.floor=floor;r.unitsPerMetre=Upm;r.radius=.045f;r.code='h'|'p'<<8|'1'<<16;
    return r;
}
// Run the world at a 90 Hz display rate; returns the time reached.
double Run(World& world,double from,double seconds,Vec player={},void (*each)(const World&)=nullptr) {
    double t=from;
    for(int i=0;i<int(seconds*90);++i){t+=1/90.;world.Advance(t,player);if(each)each(world);}
    return t;
}
Shown First(const World& world) {std::array<Shown,MaxBottles> s{};assert(world.Show(s)>=1);return s[0];}
}
int main() {
    // Merged walls cover each blocked cell exactly once.
    {
        Patch p;p.session=7;p.x0=100;p.z0=200;
        p.rows[2]=p.rows[3]=0b1110000;   // a 3x2 block
        p.rows[10]=1u<<31;                // a single cell at the edge
        p.rows[20]=~0u;                   // a whole row
        std::array<Rect,MaxRects> rects{};
        const unsigned n=MergeWalls(p,rects);
        assert(n==3);
        assert((rects[0]==Rect{104,202,3,2}));
        assert((rects[1]==Rect{131,210,1,1}));
        assert((rects[2]==Rect{100,220,32,1}));
        Patch check;check.x0=p.x0;check.z0=p.z0;
        for(unsigned i=0;i<n;++i)for(int z=0;z<rects[i].d;++z)for(int x=0;x<rects[i].w;++x) {
            const int cx=rects[i].x+x-p.x0,cz=rects[i].z+z-p.z0;
            assert(!((check.rows[cz]>>cx)&1u));check.rows[cz]|=1u<<cx;
        }
        assert(check.rows==p.rows);
        Patch board;for(int z=0;z<PatchCells;++z)board.rows[z]=z&1?0xaaaaaaaau:0x55555555u;
        assert(MergeWalls(board,rects)==MaxRects);
        Patch full;full.rows.fill(~0u);
        assert(MergeWalls(full,rects)==1&&rects[0].w==32&&rects[0].d==32);
    }
    // A cell's centre maps back to that cell on any floor (the floor's shear only).
    for(float floor:{-3.f,0.f,2.3f})for(int x:{-5,0,812})for(int z:{-1,4,1500}) {
        const auto c=CellAt(CellCorner(x+.5f,z+.5f,floor,Upm),floor,Upm);
        assert(c.x==x&&c.z==z);
    }
    // Releases outside a real hand's reach are refused.
    {
        assert(Valid(At({0,1,0})));
        auto r=At({0,1,0});r.session=0;assert(!Valid(r));
        r=At({0,1,0});r.code=0;assert(!Valid(r));
        r=At({std::numeric_limits<float>::quiet_NaN(),1,0});assert(!Valid(r));
        r=At({0,1,0},{MaxSpeed+1,0,0});assert(!Valid(r));
        r=At({0,1,0});r.w={0,MaxSpin+1,0};assert(!Valid(r));
        r=At({0,1,0});r.q={0,0,0,2};assert(!Valid(r));
        r=At({0,1,0});r.unitsPerMetre=0;assert(!Valid(r));
        r=At({0,-1,0});assert(!Valid(r));
        r=At({0,5,0});assert(!Valid(r));
        assert(Radius(0)==DefaultRadius&&Radius(1)==MaxRadius&&Radius(.001f)==MinRadius);
    }
    // A bottle let go of a metre up falls, bounces and comes to rest on the floor.
    {
        World world;assert(world.Ready());
        const Vec start{1000.4f,3.f,2000.6f};
        assert(world.Drop(At(start,{},2)));
        float lowest=10,bounce=0;double t=0;
        for(int i=0;i<90*4;++i) {
            t+=1/90.;world.Advance(t,start);
            const float y=First(world).p.y;
            if(y<lowest)lowest=y;
            else bounce=std::max(bounce,y-lowest);
        }
        assert(bounce>.01f); // it bounced, not just stopped dead
        const auto rest=First(world);
        // Lying on its side (radius above the floor) or standing (half its height).
        assert(rest.p.y>2+.045f-.01f&&rest.p.y<2+Height*.5f+.01f);
        assert(std::hypot(rest.p.x-start.x,rest.p.z-start.z)<.5f);
        assert(world.Count()==1);
        WallRequest want;assert(world.WallsWanted(want));
        assert(want.session==7&&want.floor==2&&want.unitsPerMetre==Upm&&std::hypot(want.center.x-rest.p.x,want.center.z-rest.p.z)<.01f);
        // Removed once its time is up.
        t=Run(world,t,Lifetime,start);
        assert(world.Count()==0);
        assert(!world.WallsWanted(want));
    }
    // Thrown along +x into a wall: it stops at the wall's face and comes back.
    {
        World world;
        const float floor=1.5f;
        const Vec start{500.2f,floor+1,800.7f};
        const auto c=CellAt(start,floor,Upm);
        Patch p;p.session=7;p.serial=1;p.x0=c.x-16;p.z0=c.z-16;
        for(int z=0;z<PatchCells;++z)p.rows[z]=1u<<20; // a wall column four cells ahead
        const float face=CellCorner(float(p.x0+20),float(c.z),floor,Upm).x;
        assert(face>start.x+1&&face<start.x+3);
        assert(world.Drop(At(start,{6,1,0},floor)));
        world.Walls(p);
        assert(world.Shapes()==1);
        float furthest=-1e9f;double t=0;
        for(int i=0;i<90*3;++i){t+=1/90.;world.Advance(t,start);furthest=std::max(furthest,First(world).p.x);}
        assert(furthest<face);           // never through the wall
        assert(furthest>face-.2f);       // it did reach it
        assert(First(world).p.x<furthest-.05f); // and came back off it
        // The same cells again build nothing new; another game's are ignored.
        world.Walls(p);assert(world.Shapes()==1);
        Patch other=p;other.session=8;other.serial=2;other.rows.fill(0);
        world.Walls(other);assert(world.Shapes()==1);
        // Opened (cleared) cells remove the wall.
        Patch open=p;open.serial=3;open.rows.fill(0);
        world.Walls(open);assert(world.Shapes()==0);
    }
    // At most MaxBottles; a different floor or a far player forgets them.
    {
        World world;
        for(unsigned i=0;i<MaxBottles+2;++i)assert(world.Drop(At({float(i)*.3f,1,0})));
        assert(world.Count()==MaxBottles);
        Run(world,0,.5);
        assert(world.Count()==MaxBottles);
        assert(world.Drop(At({0,2,0},{},1)));
        assert(world.Count()==1);
        Run(world,.5,.1,{Range+1,0,0});
        assert(world.Count()==0);
        assert(world.Drop(At({0,1,0})));
        world.Reset();assert(world.Count()==0);
        assert(!world.Drop(At({0,1,0},{MaxSpeed+1,0,0})));
        assert(world.Count()==0);
    }
    // A pause or clock jump resumes in place instead of stepping seconds at once.
    {
        World world;assert(world.Drop(At({0,1.5f,0})));
        world.Advance(1,{});world.Advance(1+1/90.,{});
        const float before=First(world).p.y;
        world.Advance(11,{});
        assert(std::fabs(First(world).p.y-before)<.05f);
    }
    std::puts("dropped bottles: ok");
}
