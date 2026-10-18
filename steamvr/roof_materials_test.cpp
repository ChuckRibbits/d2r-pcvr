// Exercise the actual detours against synthetic native material/query functions.
// No game process, game assets or graphics device is needed.
#include "../vr/roof_materials.cpp"
#undef NDEBUG
#include <cassert>
#include <string>
#include <thread>
#include <vector>
using namespace roofmaterials;
namespace {
bool active=true,signatures=false,failBuild=true;
int findInstalls=0,buildInstalls=0;
Definition flagDef{19,"IsDoubleSided",nullptr,0,{},0};
Definition otherDef{19,"OtherProperty",nullptr,0,{},0};
int32_t assetFlag=0;
int otherType=0,otherObject=0;
Material fixture{"ext_floor",0,nullptr,nullptr,&flagDef,&assetFlag};
const char* tavern="data/hd/env/model/act2/town/act2_town_building_tavern/level02_lod0.model";
bool FakeFind(const void* type,const void* object,const char* key,Member* out) {
    if (!out) return false;
    if (!std::strcmp(key,"Missing")){*out={};return false;}
    if (!type||!object) return false;
    *out={std::strcmp(key,"IsDoubleSided")?&otherDef:&flagDef,object};
    return true;
}
void* FakeBuild(const char* model,void* file,void* mesh,const Material* mat,uint32_t flags,uint32_t pass) {
    assert(file==reinterpret_cast<void*>(17) && mesh==reinterpret_cast<void*>(23));
    assert(flags==0x240 && pass==2); // both stack arguments must survive the hook
    Member out{};
    assert(Find(mat->type,mat->object,"IsDoubleSided",&out));
    int32_t result=0;std::memcpy(&result,out.value,4);
    const bool expected=active && ready.load() && Selected(model,mat->name);
    assert(result==(expected?1:assetFlag));
    if (expected) {
        assert(out.value!=&assetFlag);
        const void* retained=out.value;
        assert(Find(mat->type,mat->object,"OtherProperty",&out) && out.value==&assetFlag);
        assert(Find(&otherType,&otherObject,"IsDoubleSided",&out) && out.value==&otherObject);
        assert(!Find(mat->type,mat->object,"Missing",&out));
        Context* outer=current;
        const auto child=Build("data/hd/character/roof.model",file,mesh,mat,flags,pass);
        assert(child==reinterpret_cast<void*>(10) && current==outer);
        assert(*static_cast<const int32_t*>(retained)==1 && assetFlag==0);
    }
    return reinterpret_cast<void*>(uintptr_t(10+result));
}
void* Run(const char* path=tavern) {
    return Build(path,reinterpret_cast<void*>(17),reinterpret_cast<void*>(23),&fixture,0x240,2);
}
}
namespace d2rsig {
bool Check(uint64_t,const void*,uint32_t){return signatures;}
bool Hook(uint64_t rva,const void*,uint32_t,void* detour,void** original) {
    if (rva==FindRva){assert(detour==reinterpret_cast<void*>(&Find));++findInstalls;*original=reinterpret_cast<void*>(&FakeFind);return true;}
    assert(rva==BuildRva && detour==reinterpret_cast<void*>(&Build));++buildInstalls;
    if (failBuild)return false;
    *original=reinterpret_cast<void*>(&FakeBuild);return true;
}
}
int main() {
    for (const char* path : {tavern,"data/hd/env/model/act2/town/act2_town_building_tavern/level02.model"})
        for (const char* mat : {"ext_floor","ext_roof_tile01","ext_roof_tile02","ext_half_circle_roof","ext_twig_roof","ext_roof_trim","ext_awning01"})
            assert(Selected(path,mat));
    for(int lod=0;lod<5;++lod) {
        std::string path=tavern;path[path.find("_lod")+4]=char('0'+lod);assert(Selected(path,"ext_floor"));
    }
    // Regression: a visible ceiling left sky gaps through the tavern shell.
    // Both storeys use single-sided exterior walls/window surrounds at every LOD.
    for(int level:{1,2}) for(int lod=-1;lod<5;++lod) {
        const auto path=std::string("data/hd/env/model/act2/town/act2_town_building_tavern/level0")+
            std::to_string(level)+(lod<0?"":"_lod"+std::to_string(lod))+".model";
        for(const char* mat:{"ext_wall01a","ext_awning_shutters","ext_front_entrance","ext_rear_arch",
            "ext_half_circle_base","ext_front_window01_01","ext_front_window01_02",
            "ext_front_window02_01","ext_front_window02_02","ext_side_window01",
            "ext_overhang_window01a_01","ext_overhang_window01a_02",
            "ext_overhang_window01b_01","ext_overhang_window01b_02",
            "ext_overhang_window02_01","ext_overhang_window02_02",
            "int_plaster01a","int_plaster01b","int_plaster02a","int_plaster02b","int_dividers","int_stockroom_roof"})
            assert(Selected(path,mat));
        assert(!Selected(path,"med_props04")&&!Selected(path,"ext_wall01a_extra"));
    }
    assert(!Selected("data/hd/env/model/act2/town/other_building/level02.model","ext_wall01a"));
    assert(!Selected("data/hd/env/model/act2/town/act2_town_building_tavern/level03.model","ext_wall01a"));
    assert(!Selected("data/hd/env/model/act2/town/act2_town_building_tavern/level02_lod5.model","ext_wall01a"));
    assert(Selected("data/hd/env/model/act3/kurast/act3_kurast_roof/roof_center_lod0.model","roof01"));
    for (const char* mat : {"med_props04","ext_roof_column","ext_chimney","fireplace01a","int_wood_column","ext_wall01a_extra"})
        assert(!Selected(tavern,mat));
    for (const char* path : {"data/hd/character/roof.model","data/hd/items/roof.model",
        "data/hd/env/model/global/prop/roof.model","data/hd/env/model/act2/roof.model.bak",
        "data/hd/env/model/act2/../roof.model","data/hd/env/model/act2/proof.model",
        "data/hd/env/model/act2/town/act2_town_building_tavern/level01.model",
        "data/hd/env/model/act2/town/act2_town_building_tavern/level02_lod5.model"})
        assert(!Selected(path,"ext_floor"));
    assert(!Selected("data/hd/env/model/act2/roof.model","water"));
    char normalized[256]{};
    assert(String("DATA:HD\\ENV\\MODEL\\ACT2\\TOWN\\ACT2_TOWN_BUILDING_TAVERN\\LEVEL02.MODEL",normalized,true));
    assert(Selected(normalized,"ext_floor"));
    assert(!String(reinterpret_cast<const char*>(1),normalized));
    char unterminated[sizeof normalized];std::memset(unterminated,'x',sizeof unterminated);
    assert(!String(unterminated,normalized));

    // Every byte of each accepted native fingerprint must be checked.
    struct Fingerprint {uintptr_t rva;const uint8_t* bytes;size_t size;};
    const Fingerprint prints[]={{BuildRva,BuildSig,sizeof BuildSig},{FindRva,FindSig,sizeof FindSig},
        {0xFA17A8,BuildCallSig,sizeof BuildCallSig},{0xF9AEDA,FlagCallSig,sizeof FlagCallSig},
        {0x16317C3,FindResultSig,sizeof FindResultSig}};
    for (const auto& p:prints) for(size_t b=0;b<p.size;++b) {
        assert(!Verified([&](uintptr_t rva,const uint8_t* bytes,size_t size){
            if(rva!=p.rva)return true;
            std::vector<uint8_t> changed(p.bytes,p.bytes+p.size);changed[b]^=1;
            return size==changed.size() && !std::memcmp(bytes,changed.data(),size);
        }));
    }
    auto gate=+[](){return active;};
    Install(nullptr,gate);assert(!ready.load() && findInstalls==0 && buildInstalls==0);
    signatures=true;Install(nullptr,gate);assert(!ready.load() && findInstalls==1 && buildInstalls==1);
    // Partial installation cannot change any query result.
    Context scope{fixture.type,fixture.object};
    {Scope guard(current,&scope);Member out{};assert(Find(fixture.type,fixture.object,"IsDoubleSided",&out));assert(out.value==&assetFlag);}
    failBuild=false;Install(nullptr,gate);assert(ready.load() && findInstalls==1 && buildInstalls==2);
    assert(Run()==reinterpret_cast<void*>(11) && !current && assetFlag==0);
    fixture.name="ext_wall01a";assert(Run()==reinterpret_cast<void*>(11)&&assetFlag==0);
    fixture.name="ext_overhang_window01a_01";assert(Run()==reinterpret_cast<void*>(11)&&assetFlag==0);
    fixture.name="med_props04";assert(Run()==reinterpret_cast<void*>(10)&&assetFlag==0);
    fixture.name="ext_floor";
    active=false;assert(Run()==reinterpret_cast<void*>(10));active=true;
    Member out{};assert(Find(fixture.type,fixture.object,"IsDoubleSided",&out) && out.value==&assetFlag);
    for(int kind : {10,13,14,15,16,17,18})assert(!Override(&scope,scope.type,scope.object,"IsDoubleSided",kind,0,0));
    assert(!Override(&scope,scope.type,scope.object,"IsDoubleSided",19,2,0));
    assert(!Override(&scope,scope.type,scope.object,"IsDoubleSided",19,0,1));
    assert(!Override(&scope,scope.type,scope.object,"OtherProperty",19,0,0));
    std::thread a([](){for(int i=0;i<100;++i){assert(!current);Run();assert(!current);}});
    std::thread b([](){for(int i=0;i<100;++i){assert(!current);Run("data/hd/character/roof.model");assert(!current);}});
    a.join();b.join();assert(!current && assetFlag==0);
    Install(nullptr,gate);assert(findInstalls==1 && buildInstalls==2);
    puts("Roof materials: scoped native queries, nested/parallel loads, ABI forwarding and guards passed");
}
