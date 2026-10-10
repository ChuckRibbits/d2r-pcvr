#pragma once

namespace dlssdepth {
// GPU test reference for the native depth encoder and particle decoder.
// Production adjusts their shared frame range; it does not replace shaders.
inline constexpr char Shader[] = R"(
float ResolveDepth(float d, float4 projection, float4 range) {
    // Native movc consumes a bit flag; integer 1 in this float4 lane is a
    // denormal float, so a floating comparison would misclassify it as zero.
    bool orthographic = asuint(projection.w) != 0;
    float distance = orthographic
        ? lerp(projection.x, projection.y, d)
        : projection.x / max(d - projection.y, 0.0);
    return saturate((distance - range.z) * range.y);
}
#ifdef D2RVR_DEPTH_TEST
cbuffer TestCases : register(b0) { float4 Cases[192]; };
RWStructuredBuffer<float4> Results : register(u0);
[numthreads(64, 1, 1)]
void Main(uint3 id : SV_DispatchThreadID) {
    float4 input = Cases[id.x * 3], range = Cases[id.x * 3 + 2];
    float d = ResolveDepth(input.x, Cases[id.x * 3 + 1], range);
    // The particle shader decodes the R16_UNORM target, then fades against
    // its own view-space Z. Exercise this consumer too, not only encoding.
    float distance = round(d * 65535.0) / 65535.0 * range.x + range.z;
    Results[id.x] = float4(d, distance, saturate((distance + input.y) * input.z), 0);
}
#else
cbuffer FrameConstants : register(b10) { float4 Frame[54]; };
cbuffer CameraConstants : register(b3) { float4 Camera[38]; };
Texture2D<float> InputDepth : register(t10);
SamplerState DepthSampler : register(s0);
float Main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target0 {
    return ResolveDepth(InputDepth.Sample(DepthSampler, uv), Camera[37], Frame[53]);
}
#endif
)";
} // namespace dlssdepth
