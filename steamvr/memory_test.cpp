#include "memory.h"
#include <cstdio>
#include <cstring>

int main() {
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
    if(!VirtualFree(pages,0,MEM_RELEASE))return 5;
    if(framevr::ReadMemory(out,pages,sizeof out))return 6;
    if(framevr::ReadMemory(out,nullptr,sizeof out))return 7;
    if(framevr::WriteMemory(pages,out,sizeof out))return 11;
    // Write a result because this Proton launch does not forward Windows stdout.
    const char* path="Z:\\home\\berkybear\\diablo2-vr-port\\build-frame\\memory-test-result.txt";
    FILE* f=std::fopen(path,"w");if(!f)return 8;
    std::fputs("Checked reads and writes reject inaccessible and freed memory; valid operations succeed.\n",f);
    std::fclose(f);return 0;
}
