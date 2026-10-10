#include "billboards.h"
#include "billboard_signatures.h"
#include "sigscan.h"
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstring>
#ifdef D2RVR_NATIVE_OPENXR
#include "memory.h"
#endif

namespace billboards {
namespace {
// PopcornFX Screen/View Quad::Align share (this, view context, strided batch).
// The view-position implementation uses context+0x30 and the engine's world
// up axis. It retains native size, rotation, UVs, normals and tangent generation.
using Align = void (*)(void*, const void*, void*);
Align original=nullptr,viewAligned=nullptr;
void (*logger)(const char*)=nullptr;
bool (*wanted)()=nullptr;
bool installed=false;
SRWLOCK centerLock=SRWLOCK_INIT;
float center[3]{};ULONGLONG centerAt=0;
std::atomic<bool> reported{false};
bool Read(void* to,const void* from,size_t bytes) {
#ifdef D2RVR_NATIVE_OPENXR
    return framevr::ReadMemory(to,from,bytes);
#else
    __try { std::memcpy(to,from,bytes);return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
void ScreenAlign(void* self,const void* view,void* batch) {
    if(!wanted||!wanted()||!viewAligned||!view||!batch){original(self,view,batch);return;}
    alignas(16) float local[16];float eye[3];ULONGLONG at;
    AcquireSRWLockShared(&centerLock);std::memcpy(eye,center,sizeof eye);at=centerAt;ReleaseSRWLockShared(&centerLock);
    if(!at||GetTickCount64()-at>250||!Read(local,view,sizeof local)){original(self,view,batch);return;}
    float distance=0;
    for(int i=0;i<3;++i){const float d=local[12+i]-eye[i];distance+=d*d;}
    // Verify the native context is this VR camera before changing its position;
    // other views (shadow/UI/etc.) keep their native screen-aligned behavior.
    if(!std::isfinite(distance)||distance>4.f){original(self,view,batch);return;}
    std::memcpy(local+12,eye,sizeof eye);
    viewAligned(self,local,batch);
    if(!reported.exchange(true)&&logger)logger("VR billboards: screen-facing quads now face the shared head position; native world-up and particle animation retained");
}
}
void SetCenter(const float position[3]) {
    AcquireSRWLockExclusive(&centerLock);
    std::memcpy(center,position,sizeof center);centerAt=GetTickCount64();
    ReleaseSRWLockExclusive(&centerLock);
}
void Install(void (*log)(const char*),bool (*active)()) {
    if(installed)return;logger=log;wanted=active;
    using namespace signatures;
    if(!d2rsig::Check(ScreenRva,ScreenSig,sizeof ScreenSig)||
       !d2rsig::Check(ViewRva,ViewSig,sizeof ViewSig)||
       !d2rsig::Check(ViewPositionRva,ViewPositionSig,sizeof ViewPositionSig)||
       !d2rsig::Check(ViewRotateRva,ViewRotateSig,sizeof ViewRotateSig)||
       !d2rsig::Check(ScreenBatchRva,ScreenBatchSig,sizeof ScreenBatchSig))return;
    viewAligned=reinterpret_cast<Align>(d2rsig::Addr(ViewRva));
    installed=d2rsig::Hook(ScreenRva,ScreenSig,sizeof ScreenSig,reinterpret_cast<void*>(&ScreenAlign),reinterpret_cast<void**>(&original));
    if(installed&&logger)logger("VR billboards: verified native screen-to-view-position hook installed");
}
}
