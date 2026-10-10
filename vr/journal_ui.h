#pragma once
#include <cstdint>
namespace gamestate {
bool JournalOnly();
bool JournalRect(int32_t out[4]);
void ReadJournalRect(); // UI thread only
}
namespace hud {
bool LayerFound();
void JournalFrame(bool active,int eye);
// Owned copy of UI only, in R16G16B16A16_FLOAT, shader-resource state.
// RGB is premultiplied; the opaque book uses a black backing (no alpha blend).
uint64_t JournalTexture(uint32_t& width,uint32_t& height,int32_t crop[4]);
}
