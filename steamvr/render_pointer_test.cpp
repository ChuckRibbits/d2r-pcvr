// Exercise the actual user32 detours from a game-like executable. No D2R,
// runtime, player files or graphics device are loaded by this fixture.
#include "render_resolution.h"
#include <MinHook.h>
#include <cassert>
#include <cstdio>
using namespace framevr::resolution;
static LRESULT CALLBACK Native(HWND h,UINT m,WPARAM w,LPARAM l){return DefWindowProcW(h,m,w,l);}
static LRESULT CALLBACK Hook(HWND h,UINT m,WPARAM w,LPARAM l){
    LRESULT result=0;if(Handle(h,m,w,l,Native,result))return result;
    return Native(h,m,w,l);
}
static void Pump(){MSG m{};while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE))DispatchMessageW(&m);}
int main(){
    assert(MH_Initialize()==MH_OK);
    WNDCLASSW cls{};cls.lpfnWndProc=Hook;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"FramePointerFixture";
    assert(RegisterClassW(&cls));
    auto h=CreateWindowW(cls.lpszClassName,L"Pointer fixture",WS_OVERLAPPEDWINDOW,100,100,800,600,nullptr,nullptr,cls.hInstance,nullptr);
    auto other=CreateWindowW(cls.lpszClassName,L"Unrelated fixture",WS_OVERLAPPEDWINDOW,100,100,800,600,nullptr,nullptr,cls.hInstance,nullptr);
    assert(h&&other);RECT r{};assert(GetClientRect(h,&r));
    POINT origin{};assert(ClientToScreen(h,&origin));
    POINT otherOrigin{};assert(ClientToScreen(other,&otherOrigin));
    Attach(h);Update(h,true,{2160,2160},{8192,8192},{uint32_t(r.right),uint32_t(r.bottom)},4);Pump();
    POINT p{origin.x+r.right-1,origin.y+r.bottom-1};
    assert(ScreenToClient(h,&p)&&p.x==2559&&p.y==2559);
    assert(ClientToScreen(h,&p)&&p.x==origin.x+r.right-1&&p.y==origin.y+r.bottom-1);
    p={otherOrigin.x+r.right-1,otherOrigin.y+r.bottom-1};
    assert(ScreenToClient(other,&p)&&p.x==r.right-1&&p.y==r.bottom-1);
    Restore();Pump();p={origin.x+r.right-1,origin.y+r.bottom-1};
    assert(ScreenToClient(h,&p)&&p.x==r.right-1&&p.y==r.bottom-1);
    assert(MH_Uninitialize()==MH_OK);DestroyWindow(other);DestroyWindow(h);
    UnregisterClassW(cls.lpszClassName,cls.hInstance);
    std::puts("Real user32 hooks: native pointer coordinates, inverse cursor warp, unrelated window and desktop restoration passed");
}
