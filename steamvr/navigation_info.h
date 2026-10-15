#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace framevr::navigation {
// Value-only UI snapshot. Never retain a native Levels row or player pointer.
struct Location {
    int level=0,act=0; // act is displayed as 1..5; zero means unavailable
    char name[256]{};
    bool operator==(const Location&) const=default;
};
constexpr int AtlasX=768,AtlasY=400,Width=256,Height=80;
inline const char* ActLabel(int act) {
    constexpr const char* labels[]={"ACT --","ACT I","ACT II","ACT III","ACT IV","ACT V"};
    return labels[act>=1&&act<=5?act:0];
}
inline const char* AreaLabel(const Location& location) {
    return location.name[0]?location.name:"Location unavailable";
}
// D2R 3.3.93787 Levels.bin: 404-byte rows, Id byte at 4, zero-based Act
// at 13, and a 40-byte LevelName localization key at 261. Checked against
// all 138 rows in the installed game's TXT/BIN; other layouts fail closed.
template<class Read,class Localize>
Location ReadLocation(int level,uintptr_t row,uint32_t rowSize,Read read,Localize localize) {
    Location result;
    if(level<=0||level>255||row<0x10000||row>UINTPTR_MAX-404||rowSize!=404)return result;
    uint8_t id=0,act=0;char key[40]{};
    if(!read(row+4,id)||id!=level||!read(row+13,act)||act>=5||
       !read(row+261,key)||!key[0]||!std::memchr(key,0,sizeof key))return result;
    result.level=level;result.act=act+1;
    char qualified[48]{};std::snprintf(qualified,sizeof qualified,"d2r:%s",key);
    if(!localize(qualified,result.name,sizeof result.name)||!std::memchr(result.name,0,sizeof result.name))
        std::memset(result.name,0,sizeof result.name);
    return result;
}
}
