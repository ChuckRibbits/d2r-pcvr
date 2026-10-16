#pragma once
#include "../steamvr/crossbow_pose.h"
#include <array>
#include <cstdint>

namespace crossbowvr {
// Timer publishes whether native first-person motion input currently owns aim.
void Gate(bool active,uint64_t session,uint32_t equipment,uint32_t epoch);
// Copies native creation parameters; never changes the caller's request.
bool Prepare(uintptr_t params,std::array<uint8_t,0x98>& adjusted,framevr::crossbow::Launch& shot,bool server=false);
void BeginClient(const framevr::crossbow::Launch* shot);
void EndClient();
void Report();
void Created(uintptr_t unit,const framevr::crossbow::Launch& shot);
void Destroyed(uintptr_t unit);
bool Visual(uint32_t entity,uintptr_t unit,bool releasePoint);
void Transform(uintptr_t out,uint32_t entity,uintptr_t component);
}
