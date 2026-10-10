#pragma once
namespace framevr::uiworld {
void Register(void (*log)(const char*));
bool Ready();
void SetActive(bool active);
}
