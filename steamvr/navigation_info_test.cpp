#include "navigation_info.h"
#include "interaction_paint.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace framevr;
int main() {
    constexpr uintptr_t base=0x100000;
    std::array<uint8_t,404> row{};row[4]=23;
    std::memcpy(row.data()+261,"test_area",10);
    bool readable=true,localized=true;
    int reads=0,lookups=0;
    const auto read=[&](uintptr_t p,auto& value){
        ++reads;if(!readable||p<base||p-base+sizeof value>row.size())return false;
        std::memcpy(&value,row.data()+p-base,sizeof value);return true;
    };
    const auto localize=[&](const char* key,char* output,uint32_t size){
        ++lookups;assert(!std::strcmp(key,"d2r:test_area"));
        std::snprintf(output,size,"Test area floor 3");return localized;
    };
    for(int act=0;act<5;++act) {
        row[13]=uint8_t(act);const auto info=navigation::ReadLocation(23,base,404,read,localize);
        assert(info.level==23&&info.act==act+1&&!std::strcmp(info.name,"Test area floor 3"));
        assert(std::strcmp(navigation::ActLabel(info.act),"ACT --"));
    }
    assert(!std::strcmp(navigation::ActLabel(1),"ACT I")&&!std::strcmp(navigation::ActLabel(5),"ACT V"));
    assert(!std::strcmp(navigation::ActLabel(-1),"ACT --")&&!std::strcmp(navigation::ActLabel(6),"ACT --"));
    reads=lookups=0;
    for(int size:{0,300,396,403,405})assert(navigation::ReadLocation(23,base,size,read,localize)==navigation::Location{});
    for(int level:{-1,0,256})assert(navigation::ReadLocation(level,base,404,read,localize)==navigation::Location{});
    assert(navigation::ReadLocation(23,0,404,read,localize)==navigation::Location{});
    assert(navigation::ReadLocation(23,UINTPTR_MAX-100,404,read,localize)==navigation::Location{});
    assert(!reads&&!lookups); // unsupported rows/addresses never reach native readers
    assert(navigation::ReadLocation(24,base,404,read,localize)==navigation::Location{});
    row[13]=5;assert(navigation::ReadLocation(23,base,404,read,localize)==navigation::Location{});row[13]=0;
    readable=false;assert(navigation::ReadLocation(23,base,404,read,localize)==navigation::Location{});readable=true;
    assert(!lookups);
    row[261]=0;assert(navigation::ReadLocation(23,base,404,read,localize)==navigation::Location{});
    std::memset(row.data()+261,'x',40);assert(navigation::ReadLocation(23,base,404,read,localize)==navigation::Location{});
    assert(!lookups);std::memset(row.data()+261,0,40);std::memcpy(row.data()+261,"test_area",10);
    localized=false;const auto unavailable=navigation::ReadLocation(23,base,404,read,localize);
    assert(unavailable.level==23&&unavailable.act==1&&!unavailable.name[0]);
    assert(!std::strcmp(navigation::AreaLabel(unavailable),"Location unavailable"));
    const auto unterminated=navigation::ReadLocation(23,base,404,read,[](const char*,char* out,uint32_t size){
        std::memset(out,'x',size);return true;
    });
    assert(!unterminated.name[0]);
    localized=true;
    interaction::Snapshot snapshot;snapshot.session=1;snapshot.at=1000;
    snapshot.location=navigation::ReadLocation(23,base,404,read,localize);
    interaction::Output output;const auto painted=interaction::Context(snapshot,output,true);
    assert(interaction::LocationImageCurrent(painted,snapshot));
    // Old text must disappear as soon as an area/act/session changes, even
    // while the background painter still has the old atlas in flight.
    snapshot.location.level=24;assert(!interaction::LocationImageCurrent(painted,snapshot));
    snapshot.location=painted.location;snapshot.location.act=2;assert(!interaction::LocationImageCurrent(painted,snapshot));
    snapshot.location=painted.location;snapshot.location.name[0]='X';assert(!interaction::LocationImageCurrent(painted,snapshot));
    snapshot.location={};assert(!interaction::LocationImageCurrent(painted,snapshot));
    snapshot.location=painted.location;snapshot.session=2;assert(!interaction::LocationImageCurrent(painted,snapshot));
    snapshot.session=1;assert(!interaction::LocationImageCurrent(interaction::Context(snapshot,output,false),snapshot));
    assert(!interaction::Fresh(snapshot,1,1651)&&!interaction::Fresh(snapshot,1,999));
    std::puts("Navigation info: acts I-V, guarded native rows, localization failures and stale area/session images passed");
}
