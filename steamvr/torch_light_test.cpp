#include "torch_light.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace framevr::torch;
using namespace framevr::math;

static void TestTipCalculation() {
    const Vec handPos{ 0.5f, 1.2f, -0.3f };
    const Quat identity{ 0.0f, 0.0f, 0.0f, 1.0f };
    const Vec tip = ComputeTorchTip(handPos, identity, DefaultShaftOffset);
    
    // In identity orientation, offset is added directly
    assert(std::fabs(tip.x - (handPos.x + DefaultShaftOffset.x)) < 1e-4f);
    assert(std::fabs(tip.y - (handPos.y + DefaultShaftOffset.y)) < 1e-4f);
    assert(std::fabs(tip.z - (handPos.z + DefaultShaftOffset.z)) < 1e-4f);

    // 90 deg Yaw rotation (looking right)
    const Quat yaw90 = Yaw(1.57079632679f);
    const Vec tipYaw = ComputeTorchTip(handPos, yaw90, DefaultShaftOffset);
    assert(std::isfinite(tipYaw.x) && std::isfinite(tipYaw.y) && std::isfinite(tipYaw.z));
    // Y (vertical) should remain unaffected by pure yaw
    assert(std::fabs(tipYaw.y - (handPos.y + DefaultShaftOffset.y)) < 1e-4f);
}

static void TestFlameDirection() {
    // Stationary hand: flame points straight up
    const Vec stationaryVel{ 0.0f, 0.0f, 0.0f };
    const Vec flameUp = ComputeFlameDirection(stationaryVel);
    assert(std::fabs(flameUp.x) < 1e-4f);
    assert(std::fabs(flameUp.y - 1.0f) < 1e-4f);
    assert(std::fabs(flameUp.z) < 1e-4f);

    // Hand swinging forward quickly (+Z direction velocity): flame should drag backward (-Z)
    const Vec swingForward{ 0.0f, 0.0f, 2.5f };
    const Vec flameDrag = ComputeFlameDirection(swingForward, 0.2f);
    assert(flameDrag.z < -0.1f); // Flame tilts backward
    assert(flameDrag.y > 0.5f);  // Still buoyant upward
    const float len = std::sqrt(Dot(flameDrag, flameDrag));
    assert(std::fabs(len - 1.0f) < 1e-3f); // Must be normalized
}

static void TestFlameFlicker() {
    float minWave = 100.0f;
    float maxWave = -100.0f;

    // Sample across 5 seconds at 10ms intervals
    for (uint64_t t = 0; t < 5000; t += 10) {
        const float wave = ComputeFlameFlickerWave(t);
        assert(wave >= -1.0f && wave <= 1.0f);
        minWave = std::min(minWave, wave);
        maxWave = std::max(maxWave, wave);

        const float radius = ComputeFlickerRadius(DefaultBaseRadius, wave);
        assert(radius >= MinFlickerRadius && radius <= MaxFlickerRadius);
        assert(radius >= DefaultBaseRadius * 0.93f && radius <= DefaultBaseRadius * 1.07f);

        float outColor[3]{};
        ComputeFlickerColor(DefaultTorchColor, wave, outColor);
        assert(outColor[0] > 0.0f && outColor[0] <= 1.0f);
        assert(outColor[1] > 0.0f && outColor[1] <= 1.0f);
        assert(outColor[2] > 0.0f && outColor[2] <= 1.0f);
    }

    // Verify dynamic variation actually occurs
    assert(minWave < -0.5f);
    assert(maxWave > 0.5f);
}

static void TestWorldCoordinates() {
    const Vec localHead{ 0.0f, 1.7f, 0.0f };
    const Vec localTip{ 0.3f, 1.5f, -0.4f }; // Offhand torch held slightly to the right/forward
    const Vec heroWorld{ 100.0f, 6.0f, 200.0f };
    constexpr float unitsPerMetre = 2.0f;

    const Vec worldTip = LocalToWorldGrid(localTip, heroWorld, localHead, unitsPerMetre);
    assert(std::fabs(worldTip.x - (100.0f + 0.3f * unitsPerMetre)) < 1e-4f);
    assert(std::fabs(worldTip.y - (6.0f - 0.2f * unitsPerMetre)) < 1e-4f);
    assert(std::fabs(worldTip.z - (200.0f - 0.4f * unitsPerMetre)) < 1e-4f);
}

static void TestClubCodeRecognition() {
    assert(IsTorchClubCode("clb"));
    assert(IsTorchClubCode("clb "));
    assert(IsTorchClubCode("spc"));
    assert(IsTorchClubCode("spc "));
    assert(!IsTorchClubCode("ssd"));
    assert(!IsTorchClubCode("buc"));
    assert(!IsTorchClubCode("7cr"));
    assert(!IsTorchClubCode(""));
    assert(!IsTorchClubCode("c"));
}

static void TestTorchQualification() {
    // 1. Plain standard club without any fire: NOT qualified as a burning torch
    const auto plainClub = EvaluateQualification("clb ", 2 /* Normal */, 0 /* sockets */, false /* enchant */, false /* dev */);
    assert(!IsTorchQualified(static_cast<uint32_t>(plainClub)));
    assert(plainClub == TorchQualification::BaseClub);

    // 2. Club with Sorceress Enchant active: QUALIFIED
    const auto enchantedClub = EvaluateQualification("clb ", 2, 0, true /* enchant */, false);
    assert(IsTorchQualified(static_cast<uint32_t>(enchantedClub)));
    assert((static_cast<uint32_t>(enchantedClub) & static_cast<uint32_t>(TorchQualification::EnchantBuff)) != 0);

    // 3. Unique club Felloak: QUALIFIED
    const auto felloak = EvaluateQualification("clb ", 7 /* Unique */, 0, false, false);
    assert(IsTorchQualified(static_cast<uint32_t>(felloak)));
    assert((static_cast<uint32_t>(felloak) & static_cast<uint32_t>(TorchQualification::UniqueFelloak)) != 0);

    // 4. Socketed club containing a Ruby or Ral rune (socketedCount > 0): QUALIFIED
    const auto socketedClub = EvaluateQualification("clb ", 2, 1 /* 1 socketed item */, false, false);
    assert(IsTorchQualified(static_cast<uint32_t>(socketedClub)));
    assert((static_cast<uint32_t>(socketedClub) & static_cast<uint32_t>(TorchQualification::SocketedFire)) != 0);

    // 5. Dev mode test override on plain club: QUALIFIED
    const auto devClub = EvaluateQualification("clb ", 2, 0, false, true /* dev */);
    assert(IsTorchQualified(static_cast<uint32_t>(devClub)));

    // 6. Non-club weapon (e.g., Short Sword "ssd ") with enchant: NOT a torch (it is an enchanted blade, not a torch club)
    const auto enchantedSword = EvaluateQualification("ssd ", 2, 0, true, false);
    assert(!IsTorchQualified(static_cast<uint32_t>(enchantedSword)));
    assert(enchantedSword == TorchQualification::None);

    // 7. Spiked Club with sockets: QUALIFIED
    const auto spikedSocketed = EvaluateQualification("spc ", 2, 2, false, false);
    assert(IsTorchQualified(static_cast<uint32_t>(spikedSocketed)));
}

int main() {
    TestTipCalculation();
    TestFlameDirection();
    TestFlameFlicker();
    TestWorldCoordinates();
    TestClubCodeRecognition();
    TestTorchQualification();
    std::cout << "torch_light_test: All Phase 1 & Phase 2 tests passed successfully.\n";
    return 0;
}
