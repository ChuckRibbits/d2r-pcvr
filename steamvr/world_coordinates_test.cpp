#include "world_coordinates.h"
#include "melee_contact.h"
#include <cassert>
#include <limits>

using namespace framevr;
using math::Vec;
int main() {
    struct Capture {Vec render,server;float upm;};
    // Captured Windows cave positions. The old comparison rejects each one;
    // removing the renderer's elevation shear preserves ordinary native lag.
    const Capture cave[]={
        {{15086.722f,3.030f,10363.470f},{15082.983f,0,10359.764f},3.8899f},
        {{15086.367f,3.030f,10361.093f},{15082.193f,0,10357.803f},3.8899f},
        {{15081.208f,3.030f,10341.549f},{15077.485f,0,10337.843f},3.8899f},
        {{15081.331055f,3.042981f,10365.330078f},{15077.417175f,0,10361.417175f},3.8899f},
    };
    for(const auto& c:cave) {
        const auto saved=c.render*(1/c.upm);
        assert(std::hypot(c.server.x/c.upm-saved.x,c.server.z/c.upm-saved.z)>.65f);
        assert(coordinates::MatchesPlayer(c.server,saved,c.upm));
        assert(!coordinates::MatchesPlayer(c.server+Vec{c.upm*2,0,0},saved,c.upm));
        assert(!coordinates::MatchesPlayer(c.server+Vec{0,0,-c.upm*2},saved,c.upm));
    }
    // Existing flat overworld behavior also passes, including native lag.
    assert(coordinates::MatchesPlayer({10472.997742f,0,12341.637939f},
        Vec{10472.966797f,.0328084f,12341.420898f}*.25f,4));
    for(float upm:{2.f,3.8899f,8.f})for(float floor:{-6.f,0.f,3.03f,12.f}) {
        const Vec grid{80,0,120};
        const auto render=coordinates::RenderGround(grid,floor);
        const auto saved=render*(1/upm);
        assert(coordinates::MatchesPlayer(grid,saved,upm));
        assert(coordinates::MatchesPlayer(grid+Vec{upm*.64f,0,0},saved,upm));
        assert(!coordinates::MatchesPlayer(grid+Vec{upm*.66f,0,0},saved,upm));
        assert(!coordinates::MatchesPlayer(grid+Vec{upm*.5f,0,upm*.5f},saved,upm));
        // An area change may alter the terrain height too. The old pose must
        // keep its old ground transform rather than inherit the new height.
        const auto warped=coordinates::RenderGround(grid+Vec{upm*2,0,upm*2},floor+6);
        assert(!coordinates::MatchesPlayer(coordinates::GridPlane(warped),saved,upm));
        // Shift the enemy capsule as well as the guard. Otherwise the fixed
        // guard still leaves cave contacts >1 m away from the visible enemy.
        const auto ground=coordinates::RenderGround((grid+Vec{0,0,-3})*(1/upm),floor/upm);
        const float radius=.2f;
        const melee::Body target{ground+Vec{0,radius,0},ground+Vec{0,1.5f,0},radius};
        const melee::Blade before{ground+Vec{-.6f,.8f,-.3f},ground+Vec{-.6f,.8f,.3f},.04f};
        const melee::Blade after{ground+Vec{.6f,.8f,-.3f},ground+Vec{.6f,.8f,.3f},.04f};
        assert(melee::Touches(before,after,target));
        if(std::fabs(floor)/upm>1) {
            const auto delta=Vec{floor/upm*coordinates::GroundShear,0,floor/upm*coordinates::GroundShear};
            const melee::Body old{target.bottom-delta,target.top-delta,radius};
            assert(!melee::Touches(before,after,old));
        }
        const melee::Blade missBefore{before.base+Vec{0,0,1},before.tip+Vec{0,0,1},.04f};
        const melee::Blade missAfter{after.base+Vec{0,0,1},after.tip+Vec{0,0,1},.04f};
        assert(!melee::Touches(missBefore,missAfter,target));
    }
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    for(float bad:{nan,inf,0.f,-1.f,13.f})assert(!coordinates::MatchesPlayer({}, {},bad));
    assert(!coordinates::MatchesPlayer({nan,0,0},{},4));
    assert(!coordinates::MatchesPlayer({},{0,inf,0},4));
}
