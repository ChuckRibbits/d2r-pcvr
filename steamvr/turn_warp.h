#pragma once
#include "frame_math.h"
#include <array>
#include <cmath>
#include <d3d12.h>
#include <wrl/client.h>

namespace framevr {
// Eyes by turns: the eye not drawn this frame is turned on the GPU from the
// head it was drawn at to this frame's, and goes to the runtime at this
// frame's pose, both eyes at one head as from a pair. Handed in at its own
// older pose instead, it jittered and lost convergence in every head turn on
// SteamVR (owner's Steam Frame, 2026-10-19). Rotation only, as the runtime's
// own reprojection: the eyes' few millimetres of travel in a turn stay.
//
// This frame's image coordinates (u right, v down, 0..1) to the drawn image's,
// homogeneous and row-major. OpenXR poses and FOVs: -Z ahead, +Y up.
inline std::array<float,9> TurnHomography(math::Quat drawn,math::Fov drawnFov,math::Quat now,math::Fov nowFov) {
    const auto multiply=[](const float* a,const float* b) {
        std::array<float,9> m{};
        for(int r=0;r<3;++r)for(int c=0;c<3;++c)m[r*3+c]=a[r*3]*b[c]+a[r*3+1]*b[3+c]+a[r*3+2]*b[6+c];
        return m;
    };
    // this frame's pixel to its ray in this frame's view
    const float l=std::tan(nowFov.left),r=std::tan(nowFov.right),u=std::tan(nowFov.up),d=std::tan(nowFov.down);
    const float ray[9]={r-l,0,l, 0,d-u,u, 0,0,-1};
    // that ray in the drawn view: the columns are this frame's axes there
    const auto turn=math::Multiply(math::Conjugate(drawn),now);
    const auto x=math::Rotate(turn,{1,0,0}),y=math::Rotate(turn,{0,1,0}),z=math::Rotate(turn,{0,0,1});
    const float axes[9]={x.x,y.x,z.x, x.y,y.y,z.y, x.z,y.z,z.z};
    // the drawn view's ray to its pixel, times its depth along -Z
    const float L=std::tan(drawnFov.left),R=std::tan(drawnFov.right),U=std::tan(drawnFov.up),D=std::tan(drawnFov.down);
    const float pixel[9]={1/(R-L),0,L/(R-L), 0,-1/(U-D),-U/(U-D), 0,0,-1};
    const auto inDrawn=multiply(axes,ray);
    return multiply(pixel,inDrawn.data());
}

class TurnWarp {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> sourceHeap,targetHeap;
    DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
public:
    // format: the eye swapchain's typed format, of the source and the target alike.
    bool Initialize(ID3D12Device*,DXGI_FORMAT format,void (*log)(const char*));
    bool Ready() const {return pipeline!=nullptr;}
    // Render will take these: checked before a swapchain image is acquired for it.
    bool Fits(ID3D12Resource* source,ID3D12Resource* target) const;
    // source in PIXEL_SHADER_RESOURCE, target in RENDER_TARGET. slot 0 or 1, one
    // per eye: the GPU may still read the other eye's from the frame before.
    bool Render(ID3D12Device*,ID3D12GraphicsCommandList*,ID3D12Resource* source,ID3D12Resource* target,const std::array<float,9>& homography,int slot);
};
}
