#include "../vr/portal_billboard.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <vector>
using namespace billboards::portal;
void Check(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
bool Near(Vec a,Vec b){return std::fabs(a.x-b.x)<1e-5f&&std::fabs(a.y-b.y)<1e-5f&&std::fabs(a.z-b.z)<1e-5f;}
struct Memory {
    std::map<uint64_t,std::vector<unsigned char>> blocks;
    bool Read(void* out,uint64_t address,size_t n) const {
        auto i=blocks.upper_bound(address);if(i==blocks.begin())return false;--i;
        const uint64_t offset=address-i->first;
        if(offset>i->second.size()||n>i->second.size()-offset)return false;
        std::memcpy(out,i->second.data()+offset,n);return true;
    }
    template<class T>void Put(uint64_t at,const T& value){std::vector<unsigned char> b(sizeof value);std::memcpy(b.data(),&value,sizeof value);blocks[at]=b;}
};
int main(){
    Vec d;
    Check(Direction({0,6,0},{0,0,-10},d)&&Near(d,{0,0,-1}),"front portal, upright");
    Check(Direction({-5,6,0},{5,0,0},d)&&Near(d,{1,0,0}),"portal to right");
    for(float h:{.1f,1.f,6.f,20.f})
        Check(Direction({-3,h,-4},{0,0,0},d)&&Near(d,{.6f,0,.8f}),"head height never tilts portal");
    Vec a,b;
    Check(Direction({2,6,3},{5,0,7},a)&&Direction({102,26,203},{105,20,207},b)&&Near(a,b),"world translation invariance");
    Check(Direction({0,5,-10},{0,0,0},a)&&Direction({-10,5,0},{0,0,0},b)&&Near(a,{0,0,1})&&Near(b,{1,0,0}),"walking around faces new position");
    Check(!Direction({0,5,0},{0,0,0},d),"directly above avoids unstable orientation");
    Check(!Direction({std::numeric_limits<float>::quiet_NaN(),0,0},{0,0,1},d),"invalid pose rejected");
    Check(Asset("data/hd/vfx/particles/objects/vfx_only/town_portal/vfx_town_portal_NewStuff.particles"),"native portal path");
    Check(Asset("DATA\\HD\\VFX\\PARTICLES\\OBJECTS\\VFX_ONLY\\TOWN_PORTAL\\vfx_town_portal_chronicle.PARTICLES"),"native case and separator variants");
    Check(!Asset("data/hd/vfx/particles/env/act1/outdoors/fog.particles")&&!Asset("data/hd/vfx/particles/objects/vfx_only/town_portal/../fog.particles"),"other effects excluded");
    Check(CenterAsset("data/hd/vfx/particles/objects/vfx_only/town_portal/vfx_town_portal_NewStuff.particles")&&
        !CenterAsset("data/hd/vfx/particles/objects/vfx_only/town_portal/vfx_town_portal_chronicle.particles"),"center envelope scoped to captured asset variant");
    // Two different native center offsets from the same active town portal.
    Vec first,second;
    Check(CenterOffset({2.90441537f,4.00589752f,.739581585f},first)&&
        CenterOffset({.773464441f,4.00589752f,-2.89777756f},second)&&Near(first,second)&&
        Near(first,{0,4,0}),"captured outdoor centers coincide at authored anchor");
    // Return endpoint: the same asset/identity transform, but one mesh keeps
    // a pitched offset. Its horizontal radius and height both failed the old
    // check. These inputs were captured from the owner's separated town end.
    for(Vec sample:{Vec{-2.24414873f,3.98646307f,-1.98626637f},
                    Vec{-2.32459331f,3.43408847f,1.80885065f},
                    Vec{-2.32550025f,3.47525477f,1.80885065f},
                    Vec{-2.24503779f,4.02918434f,-1.98708749f}})
        Check(CenterOffset(sample,first)&&Near(first,{0,4,0}),"captured return endpoint uses same center in all three axes");
    for(float yaw:{0.f,.5f,1.5f,3.f,5.f})for(float pitch:{-1.57f,-.7f,0.f,.4f,1.57f}) {
        const Vec offset{3.f*std::cos(pitch)*std::cos(yaw),4.f+3.f*std::sin(pitch),3.f*std::cos(pitch)*std::sin(yaw)};
        Check(CenterOffset(offset,first)&&Near(first,{0,4,0}),"center invariant to yaw and pitch of native offset");
    }
    for(float radius:{2.974f,3.026f,2.f,4.f})
        Check(!CenterOffset({radius,4,0},first),"positions outside narrow center shell retained");
    Check(!CenterOffset({0,0,3},first)&&!CenterOffset({2,4,0},first)&&
        !CenterOffset({0,6,3},first)&&!CenterOffset({0,4,std::numeric_limits<float>::infinity()},first),"ground FX, non-center particles and invalid inputs retained");
    constexpr uint64_t base=0x140000000,scene=0x10000,pages=0x20000,page=0x30000,instance=0x40000,definition=0x50000,transform=0x60000,compiled=0x70000,path=0x80000;
    Memory m;auto read=[&](void* out,uint64_t at,size_t n){return m.Read(out,at,n);};
    std::array<uint64_t,3> pool{pages,0,(uint64_t(1024)<<32)|2};m.Put(scene+0x230,pool);
    std::array<uint64_t,2> pagelist{0,page};m.Put(pages,pagelist);
    std::array<uint64_t,512> slots{};slots[7]=instance;m.Put(page,slots);
    std::array<uint64_t,10> ih{};ih[0]=base+0x1da1118;ih[8]=definition;ih[9]=transform;m.Put(instance,ih);
    std::array<uint64_t,10> dh{};dh[0]=base+0x1da0ff0;dh[9]=compiled;m.Put(definition,dh);
    std::array<uint64_t,3> ch{base+0x1da67f0,1,path};m.Put(compiled,ch);
    m.Put(path,base+0x1d9d258);
    std::array<char,160> name{};std::strcpy(name.data(),"data/hd/vfx/particles/objects/vfx_only/town_portal/vfx_town_portal_NewStuff.particles");m.Put(path+32,name);
    std::array<float,16> matrix{1,0,0,0,0,1,0,0,0,0,1,0,9370,0,11290,1};m.Put(transform,matrix);
    Context c;c.scene=scene;c.instance=519;
    Instance result;Vec origin;
    Check(Resolve(read,c,0,base,result)&&result.address==instance&&IsPortal(read,result.definition,base)&&Origin(read,result,origin)&&Near(origin,{9370,0,11290}),"segmented portal instance and authored origin");
    Check(IsPortal(read,definition,base,true),"verified definition permits center envelope");
    {
        Vec points[4]={{3,4,0},{3,4,0},{3,4,0},{3,4,0}};unsigned classifications=0;
        auto classify=[&](uint64_t d){++classifications;return d==definition;};
        Check(CorrectCenters(points,4,c,base,read,classify)&&classifications==1,"constant batch classifies the portal once");
        constexpr uint64_t other=0xa0000;auto otherHeader=ih;otherHeader[8]=definition+1;m.Put(other,otherHeader);
        slots[8]=other;m.Put(page,slots);
        std::array<int32_t,4> mixed{519,520,519,520};Context batch=c;batch.instance=-1;batch.count=4;batch.indices=0xb0000;m.Put(batch.indices,mixed);
        for(auto& p:points)p={3,4,0};classifications=0;
        Check(CorrectCenters(points,4,batch,base,read,classify)&&classifications==2,"mixed batch caches by instance rather than applying first lane to all");
        Check(Near(points[0],{0,4,0})&&Near(points[2],{0,4,0})&&Near(points[1],{3,4,0})&&Near(points[3],{3,4,0}),"non-portal lanes retain native offsets");
        mixed={-1,131072,999,519};m.Put(batch.indices,mixed);for(auto& p:points)p={3,4,0};
        Check(CorrectCenters(points,4,batch,base,read,classify)&&Near(points[0],{3,4,0})&&Near(points[1],{3,4,0})&&Near(points[2],{3,4,0})&&Near(points[3],{0,4,0}),"invalid lanes do not contaminate a valid portal lane");
    }
    std::array<int32_t,2> ids{519,-1};c.instance=-1;c.indices=0x90000;c.count=2;m.Put(c.indices,ids);
    Check(Resolve(read,c,0,base,result)&&!Resolve(read,c,1,base,result)&&!Resolve(read,c,2,base,result),"mixed particle lanes and bounds");
    c.instance=1024;Check(!Resolve(read,c,0,base,result),"missing pool page rejected");
    c.instance=519;ih[0]=0;m.Put(instance,ih);Check(!Resolve(read,c,0,base,result),"reused/wrong instance rejected");
    matrix[15]=0;m.Put(transform,matrix);Check(!Origin(read,result,origin),"invalid matrix rejected");
    name.fill('x');m.Put(path+32,name);Check(!IsPortal(read,definition,base),"unterminated path rejected");
    m.blocks.erase(compiled);Check(!IsPortal(read,definition,base),"unreadable definition rejected");
    std::puts("Portal facing: upright positional direction, native asset scope, mixed instances, stale/invalid reads passed");
}
