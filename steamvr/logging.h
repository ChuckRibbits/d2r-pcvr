#pragma once
#include <windows.h>
namespace framevr::logging {
bool Start(HMODULE plugin);
void Write(const char* text);
void Stop();
}
