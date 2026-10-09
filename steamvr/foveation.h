#pragma once
#include <cstdint>
struct ID3D12Device;
struct ID3D12CommandQueue;
namespace reshade::api {struct command_list;}
namespace framevr::foveation {
void Register(void (*log)(const char*));
void Initialize(ID3D12Device*,ID3D12CommandQueue*,uint32_t width,uint32_t height);
void Configure(bool enabled,float innerRadius);
void BeginEye(int eye,float centerX,float centerY);
void EndWorld();
void Suspend(reshade::api::command_list*);
void Shutdown();
struct Stats {uint32_t shadedDraws=0,fullDraws=0;};
Stats TakeStats();
}
