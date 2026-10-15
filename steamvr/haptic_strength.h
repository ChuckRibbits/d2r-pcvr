#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace framevr::haptics {
constexpr float DefaultStrength=100,MaxStrength=200,StrengthStep=10;
enum class Category { Melee, BowDraw, BowRelease, Spell, Interaction, Alert, Count };
constexpr int CategoryCount=int(Category::Count);
struct Strengths {
    float master=DefaultStrength;
    std::array<float,CategoryCount> category{100,100,100,100,100,100};
};
// gameplay::Output feedback IDs: selection/nock, release, potion,
// string proximity, full draw, melee readiness. A potion stays Interaction
// even when the player has a bow equipped.
inline Category MotionCategory(unsigned event,bool bowReady,bool javelinReady) {
    if(event==6)return Category::Melee;
    if(event==3)return Category::Interaction;
    if(event==4||event==5||(bowReady&&(event==1||event==2)))return event==2?Category::BowRelease:Category::BowDraw;
    if(javelinReady&&(event==1||event==2))return Category::BowRelease;
    return Category::Interaction;
}
struct Sample {float amplitude;int milliseconds;float frequency=0;};
inline constexpr std::array<Sample,CategoryCount> Samples{{{.95f,10,180},{.35f,25},{.65f,45},{.4f,25},{.45f,45},{.25f,100}}};
inline float NormalizeStrength(float percent) {
    if(!std::isfinite(percent))return DefaultStrength;
    return std::round(std::clamp(percent,0.f,MaxStrength)/StrengthStep)*StrengthStep;
}
inline float ScaleAmplitude(float amplitude,float master,float category=DefaultStrength) {
    if(!std::isfinite(amplitude))return 0;
    return std::clamp(std::clamp(amplitude,0.f,1.f)*(NormalizeStrength(master)/100.f)*(NormalizeStrength(category)/100.f),0.f,1.f);
}
}
