#include "roof_materials.h"
#include "roof_material_rules.h"
#include "../cleanroom/sigscan/sigscan.h"
#include "../steamvr/memory.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>

namespace roofmaterials {
namespace {
// Native LE64 Granny structures are packed to four bytes.
#pragma pack(push,4)
struct Material { const char* name; int mapCount; const void* maps; const void* texture;
    const void* type; const void* object; };
struct Definition { int kind; const char* name; const void* reference; int width; int extra[3]; uintptr_t reserved; };
#pragma pack(pop)
struct Member { const Definition* definition; const void* value; };
static_assert(offsetof(Material,type)==0x1c && offsetof(Material,object)==0x24 && sizeof(Material)==0x2c);
static_assert(sizeof(Definition)==0x2c && offsetof(Definition,width)==0x14);
using BuildFn=void* (*)(const char*,void*,void*,const Material*,uint32_t,uint32_t);
using FindFn=bool (*)(const void*,const void*,const char*,Member*);
BuildFn originalBuild=nullptr;
FindFn originalFind=nullptr;
void (*logger)(const char*)=nullptr;
bool (*wanted)()=nullptr;
std::atomic<bool> ready{false};
std::atomic<unsigned> reports{0};
thread_local Context* current=nullptr;
constexpr int32_t enabledValue=1;

template<class T> bool Read(const T* source,T& target) {
    return source && framevr::ReadMemory(&target,source,sizeof target);
}
// Bound every copy and do not read across the next 4 KiB page boundary.
template<size_t N> bool String(const char* source,char (&out)[N],bool canonical=false) {
    if (!source) return false;
    for (size_t at=0;at<N;) {
        const auto address=reinterpret_cast<uintptr_t>(source)+at;
        const size_t n=std::min({size_t(64),N-at,size_t(4096-(address&4095))});
        if (!framevr::ReadMemory(out+at,reinterpret_cast<const void*>(address),n)) return false;
        for (size_t i=at;i<at+n;++i) {
            const auto c=static_cast<unsigned char>(out[i]);
            if (!c) {
                if (canonical && i>=5 && std::memcmp(out,"data:",5)==0) out[4]='/';
                return true;
            }
            if (c<32 || c>=127) return false;
            if (canonical) out[i]=c=='\\' ? '/' : c>='A'&&c<='Z' ? c+('a'-'A') : c;
        }
        at+=n;
    }
    return false;
}
template<class Fn> Fn Original(Fn const volatile& slot) {
    Fn call=slot;
    for (int i=0;!call&&i<(1<<20);++i){YieldProcessor();call=slot;}
    return call;
}

bool Find(const void* type,const void* object,const char* name,Member* output) {
    auto call=Original(reinterpret_cast<FindFn const volatile&>(originalFind));
    if (!call) return false;
    const bool found=call(type,object,name,output);
    if (!found || !ready.load(std::memory_order_acquire) || !current ||
        current->type!=type || current->object!=object || !output) return found;
    char memberName[64]{};
    Member result{}; Definition definition{}; int32_t value=-1;
    if (!String(name,memberName) || std::strcmp(memberName,"IsDoubleSided") ||
        !Read(output,result) || !Read(result.definition,definition) ||
        !Read(static_cast<const int32_t*>(result.value),value) ||
        !Override(current,type,object,memberName,definition.kind,definition.width,value)) return found;
    // Redirect only the caller's result, never the shared source material.
    // The native material builder consumes its normal double-sided flag and
    // creates the appropriate culling/shader variants itself. Permanent storage
    // keeps the returned value valid even if a native consumer retains it.
    const void* replacement=&enabledValue;
    if (framevr::WriteMemory(&output->value,&replacement,sizeof replacement)) ++current->changes;
    return found;
}
void* Build(const char* model,void* file,void* mesh,const Material* material,uint32_t flags,uint32_t pass) {
    auto call=Original(reinterpret_cast<BuildFn const volatile&>(originalBuild));
    if (!call) return nullptr;
    Context scope{}; Material source{}; char path[512]{},name[128]{};
    const bool selected=ready.load(std::memory_order_acquire) && wanted && wanted() &&
        Read(material,source) && String(model,path,true) && String(source.name,name,true) && Selected(path,name);
    if (selected) {scope.type=source.type;scope.object=source.object;}
    // Nested non-roof material builds must not inherit an outer roof's scope.
    Scope guard(current,selected?&scope:nullptr);
    void* result=call(model,file,mesh,material,flags,pass);
    if (selected && reports.fetch_add(1)<24 && logger) {
        char line[832]; std::snprintf(line,sizeof line,
            "roof undersides: %s material=%s double-sided overrides=%u build=%s",
            path,name,scope.changes,result?"ok":"failed");
        logger(line);
    }
    return result;
}
}
void Install(void (*log)(const char*),bool (*nativeVr)()) {
    if (ready.load()) return;
    static bool initialized=false,findHook=false,buildHook=false,told=false;
    if (!initialized) {logger=log;wanted=nativeVr;initialized=true;}
    // Once a hook is installed its entry bytes are no longer the fingerprint.
    if (!findHook && !buildHook && !Verified([](uintptr_t rva,const uint8_t* bytes,size_t n) {
            return d2rsig::Check(rva,bytes,static_cast<uint32_t>(n));
        })) {
        if (!told && logger) {told=true;logger("roof undersides: material pipeline not verified; native culling retained (will retry)");}
        return;
    }
    if (!findHook) findHook=d2rsig::Hook(FindRva,FindSig,sizeof FindSig,
        reinterpret_cast<void*>(&Find),reinterpret_cast<void**>(&originalFind));
    if (!findHook) return;
    if (!buildHook) buildHook=d2rsig::Hook(BuildRva,BuildSig,sizeof BuildSig,
        reinterpret_cast<void*>(&Build),reinterpret_cast<void**>(&originalBuild));
    if (buildHook) {
        ready.store(true,std::memory_order_release);
        if (logger) logger("roof undersides: scoped native two-sided material hooks ready");
    }
}
}
