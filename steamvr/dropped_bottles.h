#pragma once
#include "frame_math.h"
#include "world_coordinates.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <memory>

// Emptied potion bottles let go of in the world: Box3D rigid bodies that fall,
// bounce and roll on the floor under the player and against the game's wall
// cells. Purely visual; nothing here reaches the game. Positions are rendered
// world metres (interaction::Stage::World), y up.
namespace framevr::bottles {
using math::Vec;
using math::Quat;

// A square of native collision cells (two world units each) sampled on the
// game thread around a point. Bit x of rows[z] is cell (x0+x, z0+z): a wall,
// closed door or unloaded room, by the same missile-barrier mask as arrows.
inline constexpr int PatchCells=32;
struct Patch {
    uint64_t session=0,serial=0;
    int x0=0,z0=0;
    std::array<uint32_t,PatchCells> rows{};
    bool Covers(int x,int z) const {return x>=x0&&z>=z0&&x<x0+PatchCells&&z<z0+PatchCells;}
    bool Blocked(int x,int z) const {return Covers(x,z)&&((rows[z-z0]>>(x-x0))&1u);}
};
// Cells lie on the unsheared grid plane. As for arrows, only the floor's
// shear is removed: walls stand vertically in rendered space above that floor.
struct Cell {int x=0,z=0;};
inline Cell CellAt(Vec world,float floor,float unitsPerMetre) {
    const auto g=coordinates::GridPlane({world.x,floor,world.z});
    return {int(std::floor(g.x*unitsPerMetre*.5f)),int(std::floor(g.z*unitsPerMetre*.5f))};
}
// A cell's lower (x, z) corner in world metres, drawn on that floor.
inline Vec CellCorner(float x,float z,float floor,float unitsPerMetre) {
    return coordinates::RenderGround({x*2/unitsPerMetre,0,z*2/unitsPerMetre},floor);
}
// Blocked cells merged into few boxes: long runs along a row grow into the
// following rows while each of those has the whole run blocked too. A wall
// face is then one flat box face, without seams a rolling bottle can catch on.
struct Rect {
    int x=0,z=0,w=0,d=0;
    bool operator==(const Rect&) const=default;
};
inline constexpr unsigned MaxRects=PatchCells*PatchCells/2; // a checkerboard
inline unsigned MergeWalls(const Patch& p,std::array<Rect,MaxRects>& out) {
    auto left=p.rows;unsigned n=0;
    for(int z=0;z<PatchCells;++z)while(left[z]) {
        const int x=std::countr_zero(left[z]);
        int w=std::countr_one(left[z]>>x);
        const uint32_t run=(w>=32?~0u:((1u<<w)-1))<<x;
        int d=1;while(z+d<PatchCells&&(left[z+d]&run)==run)++d;
        for(int k=0;k<d;++k)left[z+k]&=~run;
        out[n++]={p.x0+x,p.z0+z,w,d};
    }
    return n;
}

// One emptied bottle let go of. Velocities come from the hand that held it.
struct Release {
    uint64_t session=0;
    double time=0;      // interaction clock, seconds
    Vec p{},v{},w{};    // model centre (world metres), m/s, rad/s
    Quat q{0,0,0,1};    // model orientation: +Y out of the opening
    float floor=0;      // world height of the ground under the player
    float unitsPerMetre=0;
    float radius=0;     // the model's widest horizontal extent, metres
    uint32_t code=0;    // item code, selects the model
};
inline constexpr float Height=.18f;           // potion models are normalized to this
inline constexpr float MinRadius=.02f,MaxRadius=.08f,DefaultRadius=.045f;
inline constexpr float MaxSpeed=25,MaxSpin=40; // beyond a real hand's release
inline constexpr unsigned MaxBottles=6;        // the oldest goes first
inline constexpr double Lifetime=20;           // seconds after release
inline constexpr float Range=12;               // metres from the player before it is forgotten
inline constexpr float WallHeight=6;           // walls stand this high above the floor
inline constexpr double Step=1/120.;
inline bool Finite(Vec v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline bool Valid(const Release& r) {
    const float n=r.q.x*r.q.x+r.q.y*r.q.y+r.q.z*r.q.z+r.q.w*r.q.w;
    return r.session&&r.code&&std::isfinite(r.time)&&Finite(r.p)&&Finite(r.v)&&Finite(r.w)&&
        std::isfinite(n)&&std::fabs(n-1)<.01f&&std::isfinite(r.floor)&&
        std::isfinite(r.unitsPerMetre)&&r.unitsPerMetre>=1&&r.unitsPerMetre<=12&&
        math::Dot(r.v,r.v)<=MaxSpeed*MaxSpeed&&math::Dot(r.w,r.w)<=MaxSpin*MaxSpin&&
        r.p.y>r.floor-.25f&&r.p.y<r.floor+3.5f;
}
inline float Radius(float modelRadius) {
    return std::isfinite(modelRadius)&&modelRadius>0?std::clamp(modelRadius,MinRadius,MaxRadius):DefaultRadius;
}
struct Shown {Vec p{};Quat q{0,0,0,1};uint32_t code=0;};
// A held weapon, shield or fist that pushes bottles: a capsule from base to
// tip in world metres, the same posed capsule physical melee tests creatures
// with. Slots: right hand, left hand, shield.
struct Collider {
    bool active=false;
    Vec base{},tip{};
    float radius=0;
};
inline constexpr int Colliders=3;
inline constexpr float MinColliderRadius=.02f,MaxColliderRadius=.08f; // a shield's fat creature capsule pushes from too far
inline constexpr float ColliderJump=.75f;  // metres in one frame: a warp or recentre, moved without sweeping
inline constexpr float ColliderMaxSpeed=30; // m/s, the same: never swept faster
inline bool Valid(const Collider& c) {
    const Vec d=c.tip-c.base;
    return c.active&&Finite(c.base)&&Finite(c.tip)&&std::isfinite(c.radius)&&c.radius>0&&math::Dot(d,d)<=3*3;
}
// Where the game thread should sample walls: around the bottles, on their floor.
struct WallRequest {
    uint64_t session=0;
    Vec center{};
    float floor=0,unitsPerMetre=0;
};

class World {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    World();
    ~World();
    World(const World&)=delete;
    World& operator=(const World&)=delete;
    bool Ready() const; // the Box3D world exists
    // A release on another floor or game forgets the earlier bottles.
    bool Drop(const Release&);
    // The game's wall cells; a patch from another game is ignored.
    void Walls(const Patch&);
    // Advance in fixed steps to time. Bottles older than Lifetime, or farther
    // than Range from the player, are removed. Beyond the sampled cells there
    // are no walls, only the floor.
    void Advance(double time,Vec player);
    // Where the weapons are now. The next Advance moves each one there over
    // the steps it takes, pushing and waking the bottles in its way; a slot
    // without a valid collider is removed. Only while bottles lie in the world.
    void Weapons(const std::array<Collider,Colliders>&);
    // The fastest a weapon struck a bottle since the last call (m/s), cleared.
    float Struck(int slot);
    // Interpolated poses between the last two steps.
    unsigned Show(std::array<Shown,MaxBottles>& out) const;
    bool WallsWanted(WallRequest&) const;
    unsigned Count() const;
    unsigned Shapes() const; // static wall boxes, for diagnostics
    unsigned Pushers() const; // weapon bodies in the world
    void Reset();
};
}
