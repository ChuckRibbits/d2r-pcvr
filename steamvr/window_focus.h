#pragma once
#include <windows.h>
#include <atomic>

namespace framevr {
// Activating a window can synchronously call the game's WndProc. Never do it
// inside an engine/render callback: the native focus path takes game locks.
// Post one short-lived request; the owning window thread handles it later.
class DeferredWindowFocus {
    using FocusFn = BOOL (WINAPI*)(HWND);
    std::atomic<WPARAM> pending{0},serial{0};
    FocusFn focus;
    static UINT Message() {
        static const UINT message=RegisterWindowMessageW(L"D2R.PCVR.DeferredWindowFocus.v1");
        return message;
    }
public:
    explicit DeferredWindowFocus(FocusFn fn=&SetForegroundWindow):focus(fn){}
    bool Request(HWND window) {
        const UINT message=Message();
        if(!message||!window||!IsWindow(window))return false;
        const WPARAM token=serial.fetch_add(1)+1;
        WPARAM expected=0;
        if(!pending.compare_exchange_strong(expected,token))return true;
        if(PostMessageW(window,message,token,LPARAM(GetTickCount64()+500)))return true;
        expected=token;pending.compare_exchange_strong(expected,0);return false;
    }
    bool Handle(HWND window,UINT message,WPARAM token,LPARAM deadline) {
        if(!Message()||message!=Message())return false;
        WPARAM expected=token;
        if(token&&pending.compare_exchange_strong(expected,0)&&GetTickCount64()<=ULONGLONG(deadline)&&IsWindow(window))focus(window);
        return true;
    }
    void Cancel(){pending.store(0);}
};
}
