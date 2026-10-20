#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace framevr::sewerinteraction {
// D2R 3.3.93787: room lists use Unit+0x160, NOT the hash-list link +0x158.
// Only the existing Kurast -> Sewers Level 1 warp (57), never arbitrary exits.
struct Result {
    uintptr_t unit=0;
    uint32_t id=0,visited=0,tiles=0,firstTileClass=UINT32_MAX,warps=0,facing=0;
    float distance=0;
};
template<class Read,class Allowed>
Result Find(uintptr_t player,int level,float forwardX,float forwardY,Read read,Allowed allowed) {
    Result result;
    const float length=std::hypot(forwardX,forwardY);
    if((level!=80&&level!=81)||!std::isfinite(length)||length<.05f)return result;
    forwardX/=length;forwardY/=length;
    uint32_t header[4]{},xy[2]{};uintptr_t path=0,room=0;
    if(!read(player,header)||header[0]!=0||(header[3]!=1&&header[3]!=2&&header[3]!=3)||
       !read(player+0x38,path)||!read(path,xy)||!read(path+0x20,room)||room<0x10000)return result;
    const float x=xy[0]/65536.f,y=xy[1]/65536.f;
    if(x<=0||y<=0)return result;
    std::array<uintptr_t,65> rooms{};unsigned count=1;rooms[0]=room;
    uintptr_t nearby=0;uint32_t nearCount=0;
    if(!read(room,nearby)||!read(room+0x40,nearCount)||nearCount>64||(!nearby&&nearCount))return result;
    for(unsigned i=0;i<nearCount;++i) {
        uintptr_t next=0;
        if(!read(nearby+i*8,next))return result;
        if(next>=0x10000&&std::find(rooms.begin(),rooms.begin()+count,next)==rooms.begin()+count)
            rooms[count++]=next;
    }
    float best=8.f;
    for(unsigned r=0;r<count;++r) {
        uintptr_t unit=0;
        if(!read(rooms[r]+0xa8,unit))continue;
        for(unsigned chain=0;unit&&chain<256&&result.visited<1024;++chain) {
            ++result.visited;
            uintptr_t next=0;uint32_t h[3]{};
            if(!read(unit+0x160,next)||!read(unit,h))break;
            if(h[0]==5){++result.tiles;if(result.firstTileClass==UINT32_MAX)result.firstTileClass=h[1];}
            if(h[0]==5&&h[1]==57) {
                uintptr_t staticPath=0,ownerRoom=0;uint32_t position[2]{};
                if(read(unit+0x38,staticPath)&&read(staticPath,ownerRoom)&&ownerRoom==rooms[r]&&
                   read(staticPath+0x10,position)&&position[0]>0&&position[1]>0&&
                   position[0]<=65535&&position[1]<=65535) {
                    const float dx=float(position[0])-x,dy=float(position[1])-y;
                    const float distance=std::hypot(dx,dy);
                    if(distance<=8.f) {
                        ++result.warps;
                        // Ignore vertical gaze: a standing player should not
                        // need to crouch or aim at the floor to select stairs.
                        if(distance<.75f||(dx*forwardX+dy*forwardY)/distance>=.5f) {
                            ++result.facing;
                            if(distance<best&&allowed(player,unit)) {
                                best=distance;result.unit=unit;result.id=h[2];result.distance=distance;
                            }
                        }
                    }
                }
            }
            if(next==unit)break;
            unit=next;
        }
    }
    return result;
}
}
