#include "window_focus.h"
#include <cassert>

namespace {
SRWLOCK engineLock=SRWLOCK_INIT;
unsigned activations=0;
DWORD owner=0;
BOOL WINAPI Focus(HWND) {
    assert(GetCurrentThreadId()==owner);
    // A synchronous activation inside the simulated render callback fails
    // this check instead of hanging the test process on a recursive SRW lock.
    assert(TryAcquireSRWLockExclusive(&engineLock));
    ReleaseSRWLockExclusive(&engineLock);
    ++activations;return FALSE; // Windows can deny focus; retry must remain possible.
}
framevr::DeferredWindowFocus request(&Focus);
LRESULT CALLBACK Window(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(request.Handle(window,message,w,l))return 0;
    return DefWindowProcW(window,message,w,l);
}
void Pump(bool expired=false) {
    MSG message{};
    while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
        if(expired&&message.message>=0xC000)message.lParam=0;
        DispatchMessageW(&message);
    }
}
}
int main() {
    owner=GetCurrentThreadId();
    WNDCLASSW cls{};cls.lpfnWndProc=&Window;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"D2RFocusRegression";
    assert(RegisterClassW(&cls));
    HWND window=CreateWindowW(cls.lpszClassName,L"",0,0,0,0,0,HWND_MESSAGE,nullptr,cls.hInstance,nullptr);
    assert(window);
    assert(!request.Request(nullptr));
    AcquireSRWLockExclusive(&engineLock);
    for(int i=0;i<100;++i)assert(request.Request(window));
    assert(activations==0); // render callback returns without invoking native focus
    ReleaseSRWLockExclusive(&engineLock);
    Pump();assert(activations==1); // requests coalesced; runs on owner thread
    assert(request.Request(window));Pump(true);assert(activations==1); // stale requests never steal focus
    assert(request.Request(window));request.Cancel();Pump();assert(activations==1);
    assert(request.Request(window));request.Cancel();assert(request.Request(window));Pump();assert(activations==2);
    assert(request.Request(window));Pump();assert(activations==3); // denied focus can retry
    DestroyWindow(window);assert(!request.Request(window));
    UnregisterClassW(cls.lpszClassName,cls.hInstance);
}
