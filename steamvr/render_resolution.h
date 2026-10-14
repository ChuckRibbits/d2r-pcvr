#pragma once
#include <windows.h>
#include "vr_resolution.h"
namespace framevr::resolution {
void Register(void (*log)(const char*));
void Attach(HWND window);
bool Handle(HWND window,UINT message,WPARAM w,LPARAM& l,WNDPROC original,LRESULT& result);
void Update(HWND window,bool xrRunning,Size recommended,Size maximum,Size source,int choice);
void Restore();
// Called before native UI dispatch; recovery is deferred until after Apply.
void OptionsApplied();
void Describe(char* text,size_t capacity,Size source);
}
