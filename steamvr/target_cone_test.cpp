#include "target_cone.h"
#include <cstdio>
#include <cstdlib>
using namespace framevr::targetcone;
static void Check(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main() {
    // Hero at 100,100 aiming +x.
    const Target line[]={{1,120,100,true},{2,106,101,true},{3,90,100,true}};
    Check(Pick(line,3,100,100,1,0,0)==1,"the near mob in front wins over the one behind it");
    Check(Pick(line,3,100,100,-1,0,0)==2,"turning round picks the mob there");
    Check(Pick(line,3,100,100,0,1,0)==-1,"nobody inside the cone, no target");
    const Target walled[]={{1,105,100,false},{2,115,100,true}};
    Check(Pick(walled,2,100,100,1,0,0)==1,"a mob behind a wall is skipped");
    const Target far[]={{1,100+framevr::aimreach::GameCells+1,100,true}};
    Check(Pick(far,1,100,100,1,0,0)==-1,"a mob past the game's reach is never picked");
    const Target corner[]={{1,140,140,true}};   // 40 cells on each axis: 56.6 cells away, still inside the game's square
    Check(Pick(corner,1,100,100,1,1,0)==0,"the reach is the game's square, not a circle");
    // Two mobs side by side, 20 degrees apart either way of +x at 10 cells.
    const Target pair[]={{1,109.4f,103.4f,true},{2,109.4f,96.6f,true}};
    Check(Pick(pair,2,100,100,1,0,1)==0&&Pick(pair,2,100,100,1,0,2)==1,"between two equal mobs the current one stays");
    // Looking at the second one moves the lock, even though the first is still in the cone.
    const float toSecond[2]={0.94f,-0.342f};   // -20 degrees
    Check(Pick(pair,2,100,100,toSecond[0],toSecond[1],1)==1,"turning to another mob moves the lock at once");
    // A farther mob straight ahead beats a nearer one at the cone's edge.
    const Target edge[]={{1,104,102.2f,true},{2,112,100,true}};   // ~29 degrees at 4.6 cells vs 0 degrees at 12
    Check(Pick(edge,2,100,100,1,0,0)==1,"the mob being aimed at beats a nearer one at the edge");
    Check(Pick(line,3,100,100,0,0,0)==-1,"no heading, no target");
    Check(Band(4)==0&&Band(18)==1&&Band(30)==2,"color bands by distance");
    {   // a stream of monsters in hash order, kept 3: the nearest 3 stay
        float kept[3]{};int n=0;
        for(float d:{30.f,5.f,20.f,40.f,10.f,1.f}){const int slot=NearestSlot(kept,n,3,d);if(slot>=0){kept[slot]=d;if(slot==n)++n;}}
        float sum=0;for(int i=0;i<n;++i)sum+=kept[i];
        Check(n==3&&sum==16.f,"the nearest 3 of the stream are kept (1, 5 and 10)");
        Check(NearestSlot(kept,3,3,50)==-1,"a farther one is dropped when full");
    }
    std::puts("target cone tests passed");
}
