#include "bow_laser.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr;
using namespace framevr::bowlaser;
static void Check(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
static bool Near(float a,float b){return std::fabs(a-b)<1e-4f;}
int main() {
    const Vec forward{0,0,-1};
    Check(Shown(true,forward,forward)&&!Shown(false,forward,forward),"shown only at full draw");
    Check(!Shown(true,forward,{0,0,1}),"a draw that would not shoot shows no line");
    Check(!Shown(true,{NAN,0,0},forward),"an invalid direction shows no line");
    {   // Level shot: the floor is never met, the line runs to its full length.
        const auto p=Trace({0,1.5f,0},forward,true,0);
        Check(Near(p.start.z,-bowflight::ArrowLength)&&Near(p.start.y,1.5f),"the line starts at the arrow's tip");
        Check(Near(p.length,MaxLength)&&!p.floor&&Near(p.end.z,p.start.z-MaxLength),"a level shot ends at the maximum length");
    }
    {   // Aimed down 45 degrees: the flight's own floor stop, with its clearance.
        const float s=std::sqrt(.5f);const Vec down{0,-s,-s};
        const auto p=Trace({0,1.5f,0},down,true,0);
        const float above=1.5f-s*bowflight::ArrowLength-bowflight::FloorClearance;
        Check(p.floor&&Near(p.length,above/s)&&Near(p.end.y,bowflight::FloorClearance),"a downward shot ends on the launch floor");
        Check(!Trace({0,1.5f,0},down,false,0).floor,"without a known floor the line runs to its maximum");
        Check(Near(Trace({0,11.5f,0},down,true,0).length,MaxLength),"a floor beyond the maximum is not reached");
        Check(Trace({0,.3f,0},down,true,0).length==0,"a tip already at the floor shows no line");
    }
    Check(Trace({0,1.5f,0},{0,0,-2},true,0).length==0,"a non-unit direction shows no line");
    Check(Near(Trace({0,1.5f,0},{0,1,0},true,0).length,MaxLength),"an upward shot never meets the floor");
    std::puts("Bow laser: draw gate, arrow-tip start, floor stop and maximum length passed");
}
