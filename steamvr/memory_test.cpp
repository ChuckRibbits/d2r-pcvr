#include "memory.h"
#include <cstdio>
#include <cstring>
#include <atomic>

static std::atomic<unsigned> exceptions{0};
static LONG CALLBACK Observe(EXCEPTION_POINTERS*){++exceptions;return EXCEPTION_CONTINUE_SEARCH;}

int main() {
    auto observer=AddVectoredExceptionHandler(1,Observe);if(!observer)return 12;
    SYSTEM_INFO info{};GetSystemInfo(&info);
    const size_t page=info.dwPageSize;
    auto* pages=static_cast<unsigned char*>(VirtualAlloc(nullptr,page*2,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    if(!pages)return 1;
    unsigned char out[32]{};
    std::memset(pages,42,page*2);
    if(!framevr::ReadMemory(out,pages,sizeof out)||out[0]!=42)return 2;
    out[0]=7;
    if(!framevr::WriteMemory(pages,out,sizeof out)||pages[0]!=7)return 9;
    DWORD old=0;
    if(!VirtualProtect(pages+page,page,PAGE_NOACCESS,&old))return 3;
    if(framevr::ReadMemory(out,pages+page-16,sizeof out))return 4;
    if(framevr::WriteMemory(pages+page,out,sizeof out))return 10;
    if(framevr::WriteMemory(pages+page-16,out,sizeof out))return 13;
    if(framevr::WriteMemory(pages,pages+page,sizeof out))return 14;
    if(!VirtualProtect(pages+page,page,PAGE_READONLY,&old))return 15;
    if(framevr::WriteMemory(pages+page,out,sizeof out))return 16;
    if(pages[page]!=42)return 17; // never unprotect/patch a read-only destination
    if(!VirtualFree(pages,0,MEM_RELEASE))return 5;
    if(framevr::ReadMemory(out,pages,sizeof out))return 6;
    if(framevr::ReadMemory(out,nullptr,sizeof out))return 7;
    if(framevr::WriteMemory(pages,out,sizeof out))return 11;
    if(framevr::WriteMemory(nullptr,out,sizeof out))return 18;
    if(exceptions.load()!=0)return 19; // errors stay inside the OS, not D2R's crash handler
    RemoveVectoredExceptionHandler(observer);
    // Write a result because this Proton launch does not forward Windows stdout.
    const char* path="Z:\\home\\berkybear\\diablo2-vr-port\\build-frame\\memory-test-result.txt";
    FILE* f=std::fopen(path,"w");if(!f)return 8;
    std::fputs("Checked local copies succeed; inaccessible, freed, null, read-only and boundary-crossing buffers fail without entering application exception handlers.\n",f);
    std::fclose(f);return 0;
}
