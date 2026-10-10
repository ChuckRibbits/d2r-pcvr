#pragma once
#include <cstdint>
namespace gamestate {
bool JournalOnly();
bool OpaqueMenuOpen();
bool JournalRect(int32_t out[4]);
void ReadJournalRect(); // UI thread only
}
namespace hud {
bool LayerFound();
void UiFrame(bool active,bool workspace,int eye);
// Owned copy of UI only, in R16G16B16A16_FLOAT, shader-resource state.
// RGB is premultiplied, alpha is world transmittance; convert before XR use.
uint64_t UiTexture(uint32_t& width,uint32_t& height,int32_t crop[4]);
}
