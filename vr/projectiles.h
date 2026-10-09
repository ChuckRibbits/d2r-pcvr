#pragma once

namespace projectiles {
// Optional, signature-checked native motion-bow adjustments. Ordinary attacks
// from other owners and all non-basic-arrow missile types retain native rules.
void Configure(bool motionControls, float arrowSpeed, bool directRelease);
void Install();
void Report();
}
