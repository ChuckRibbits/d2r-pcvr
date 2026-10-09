#include "gaze.h"
#include "foveation_math.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace framevr;
int main() {
    constexpr float pi=3.14159265359f;
    const math::Fov centered{-pi/4,pi/4,-pi/4,pi/4};
    const math::Quat identity{0,0,0,1};gaze::UV p;
    assert(gaze::Project(identity,identity,centered,p));assert(std::abs(p.x-.5f)<1e-6f&&std::abs(p.y-.5f)<1e-6f);
    assert(gaze::Project(math::Yaw(-.3f),identity,centered,p)&&p.x>.5f);
    // Head turn applied to both poses cancels; the gaze region follows the
    // eye within the render image instead of rotating the camera itself.
    gaze::UV turned;const auto turn=math::Yaw(1.2f);
    assert(gaze::Project(math::Multiply(turn,math::Yaw(-.3f)),turn,centered,turned));assert(std::abs(p.x-turned.x)<1e-6f);
    assert(gaze::Project({std::sin(.15f),0,0,std::cos(.15f)},identity,centered,p)&&p.y<.5f);
    const math::Fov asymmetric{-.6f,.9f,-.8f,.5f};
    assert(gaze::Project(identity,identity,asymmetric,p));
    float projection[16];math::Projection(asymmetric.left,asymmetric.right,asymmetric.down,asymmetric.up,.01f,projection);
    assert(std::abs(p.x-(-projection[8]+1)*.5f)<1e-6f);
    assert(std::abs(p.y-(projection[9]+1)*.5f)<1e-6f);
    assert(!gaze::Project(math::Yaw(pi),identity,centered,p));
    assert(!gaze::Project(math::Yaw(1.f),identity,centered,p));
    assert(!gaze::Project({0,0,0,0},identity,centered,p));
    assert(!gaze::Project({0,0,0,std::numeric_limits<float>::quiet_NaN()},identity,centered,p));
    constexpr int64_t now=2'000'000'000;
    assert(gaze::Fresh(now,now-100'000'000));assert(gaze::Fresh(now,now+20'000'000));
    assert(!gaze::Fresh(now,0));assert(!gaze::Fresh(now,now-100'000'001));assert(!gaze::Fresh(now,now+20'000'001));
    gaze::Stability stability;std::array<gaze::UV,2> points{};
    assert(!stability.Update(true,points));assert(stability.Update(true,points));
    points[0].x=.8f;assert(!stability.Update(true,points));assert(stability.Update(true,points));
    assert(!stability.Update(false,points));assert(!stability.Update(true,points));assert(stability.Update(true,points));
    // Exhaustively cover a dense gaze grid and boundary directions: every
    // protected point must remain inside the quantized mask's expanded region.
    constexpr float radius=.35f;
    for(int y=0;y<=100;++y)for(int x=0;x<=100;++x) {
        const float u=x*.01f,v=y*.01f;const int cell=GazeCell(u,v);
        assert(cell>=0&&cell<GazeMaskCount);
        const float cx=GazeGridCoordinate(cell%GazeGrid),cy=GazeGridCoordinate(cell/GazeGrid);
        for(int a=0;a<32;++a) {
            const float theta=a*2*pi/32;
            const float px=u+radius*.5f*std::cos(theta),py=v+radius*.5f*std::sin(theta);
            assert(2*std::hypot(px-cx,py-cy)<=radius+GazeQuantizationMargin+1e-6f);
            if(px>=0&&px<1&&py>=0&&py<1)
                assert(FixedRate(uint32_t(px*1920)/16,uint32_t(py*1080)/16,16,0,0,1920,1080,cx,cy,radius+GazeQuantizationMargin)==0);
        }
    }
    std::puts("Gaze projection, validity, recovery and protected-region tests passed");
}
