// Exercise the production shared-range correction and native shader contract against independently derived
// camera distances, including the captured 550..1060 normalization regression.
// Run in the isolated test prefix, never the live game's Proton prefix.
#include "../vr/dlss_depth_shader.h"
#include "../vr/dlss_depth_range.h"
#include <limits>
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
struct V4 { float x,y,z,w; };
struct Case { V4 input,projection,range; };
void Check(HRESULT hr, const char* operation) {
    if (FAILED(hr)) { std::fprintf(stderr,"%s: 0x%08lx\n",operation,(unsigned long)hr); throw std::runtime_error(operation); }
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        const float vrProjection[16]={.6f,0,0,0,0,.58f,0,0,0,0,0,-1,0,0,.2f,0};
        for (int invalid=0; invalid<8; ++invalid) {
            float p[16];std::memcpy(p,vrProjection,sizeof p);
            float r[4]={510,1.f/510,550,123};
            switch(invalid) {
            case 0:p[15]=1;break; // native orthographic/menu
            case 1:p[10]=-.001f;break; // finite perspective passes through
            case 2:p[14]=0;break;
            case 3:p[0]=std::numeric_limits<float>::quiet_NaN();break;
            case 4:r[1]=0;break;
            case 5:r[0]=-1;break;
            case 6:r[2]=-1000;break;
            case 7:r[1]=1;break;
            }
            float before[4];std::memcpy(before,r,sizeof r);
            if(dlssdepth::PerspectiveRange(p,r)||std::memcmp(before,r,sizeof r))
                throw std::runtime_error("invalid/non-VR range changed");
        }
        float range[4]={510,1.f/510,550,123};
        if(!dlssdepth::PerspectiveRange(vrProjection,range)||range[3]!=123||
            std::abs(range[0]+range[2]-1060)>1e-4f)
            throw std::runtime_error("VR range/far limit/reserved lane");
        float again[4];std::memcpy(again,range,sizeof range);
        if(!dlssdepth::PerspectiveRange(vrProjection,again)||std::memcmp(again,range,sizeof range))
            throw std::runtime_error("range correction is not idempotent");
        const auto module=LoadLibraryA(argv[1]);
        const auto compile=module?reinterpret_cast<pD3DCompile>(GetProcAddress(module,"D3DCompile")):nullptr;
        if(!compile)throw std::runtime_error("compiler unavailable");
        ComPtr<ID3DBlob> pixel,compute,error;
        Check(compile(dlssdepth::Shader,std::strlen(dlssdepth::Shader),"depth",nullptr,nullptr,"Main","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&pixel,&error),"pixel compile");
        const D3D_SHADER_MACRO defines[]={{"D2RVR_DEPTH_TEST","1"},{nullptr,nullptr}};
        const HRESULT compiled=compile(dlssdepth::Shader,std::strlen(dlssdepth::Shader),"depth",defines,nullptr,"Main","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&compute,&error);
        if(FAILED(compiled)&&error)std::fprintf(stderr,"%s\n",static_cast<const char*>(error->GetBufferPointer()));
        Check(compiled,"compute compile");
        ComPtr<ID3D11Device> dev;ComPtr<ID3D11DeviceContext> ctx;D3D_FEATURE_LEVEL level;
        Check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,&level,&ctx),"device");
        ComPtr<ID3D11PixelShader> ps;ComPtr<ID3D11ComputeShader> cs;
        Check(dev->CreatePixelShader(pixel->GetBufferPointer(),pixel->GetBufferSize(),nullptr,&ps),"pixel shader");
        Check(dev->CreateComputeShader(compute->GetBufferPointer(),compute->GetBufferSize(),nullptr,&cs),"compute shader");
        std::array<Case,64> cases{};std::array<double,64> expected{};
        for(int i=0;i<32;++i) {
            // World-space points transformed by our infinite reverse-Z
            // projection produce near/distance in hardware depth.
            const double distance=i==31?2000.0:.2*std::pow(1.30,i);
            cases[i]={{float(.2/distance),float(-distance+(i%2?.1:-.1)),5,0},{.2f,0,30,0},{range[0],range[1],range[2],range[3]}};
            expected[i]=std::clamp((distance-.2)/(1060-.2),0.0,1.0);
        }
        cases[31].input.x=0; expected[31]=1; // sky/infinity
        for(int i=32;i<48;++i) {
            // Original reverse-Z orthographic interval: unchanged.
            const float d=float(i-32)/15;
            cases[i]={{d,0,0,0},{1060,550,30,1},{510,1.f/510,550,0}};
            if(i&1){const uint32_t flag=1;std::memcpy(&cases[i].projection.w,&flag,sizeof flag);}
            expected[i]=1.0-d;
        }
        for(int i=48;i<64;++i) {
            // Finite perspective: unchanged, including clamping at either end.
            const double distance=300+70*(i-48),a=10.01,b=-.001;
            cases[i]={{float(a/distance+b),0,0,0},{float(a),float(b),30,0},{510,1.f/510,550,0}};
            expected[i]=std::clamp((distance-550)/510,0.0,1.0);
        }
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=sizeof cases;cb.Usage=D3D11_USAGE_IMMUTABLE;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        D3D11_SUBRESOURCE_DATA data{cases.data(),0,0};ComPtr<ID3D11Buffer> constants,output,readback;
        Check(dev->CreateBuffer(&cb,&data,&constants),"constants");
        D3D11_BUFFER_DESC out{};out.ByteWidth=64*sizeof(V4);out.Usage=D3D11_USAGE_DEFAULT;out.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
        out.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;out.StructureByteStride=sizeof(V4);
        Check(dev->CreateBuffer(&out,nullptr,&output),"output");
        D3D11_UNORDERED_ACCESS_VIEW_DESC uav{};uav.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;uav.Buffer.NumElements=64;
        ComPtr<ID3D11UnorderedAccessView> view;Check(dev->CreateUnorderedAccessView(output.Get(),&uav,&view),"UAV");
        D3D11_BUFFER_DESC staging{};staging.ByteWidth=out.ByteWidth;staging.Usage=D3D11_USAGE_STAGING;staging.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        Check(dev->CreateBuffer(&staging,nullptr,&readback),"readback");
        ID3D11Buffer* buffers[]={constants.Get()};ID3D11UnorderedAccessView* views[]={view.Get()};
        ctx->CSSetShader(cs.Get(),nullptr,0);ctx->CSSetConstantBuffers(0,1,buffers);ctx->CSSetUnorderedAccessViews(0,1,views,nullptr);
        ctx->Dispatch(1,1,1);ctx->CopyResource(readback.Get(),output.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};Check(ctx->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped),"map");
        const auto* actual=static_cast<const V4*>(mapped.pData);int distinct=0,last=-1;
        for(int i=0;i<64;++i) {
            if(!std::isfinite(actual[i].x)||std::abs(actual[i].x-expected[i])>2e-5) {
                std::fprintf(stderr,"case %d: got %.8g expected %.8g\n",i,actual[i].x,expected[i]);return 1;
            }
            const auto& c=cases[i];
            const double decoded=std::round(expected[i]*65535.0)/65535.0*c.range.x+c.range.z;
            const double fade=std::clamp((decoded+c.input.y)*c.input.z,0.0,1.0);
            if(!std::isfinite(actual[i].y)||!std::isfinite(actual[i].z)||
                std::abs(actual[i].y-decoded)>.02||std::abs(actual[i].z-fade)>.01)
                throw std::runtime_error("particle depth reconstruction/fade mismatch");
            if(i<31){const int q=int(std::lround(actual[i].x*65535));if(q!=last)++distinct;last=q;}
        }
        ctx->Unmap(readback.Get(),0);
        if(distinct<28)throw std::runtime_error("nearby surfaces lost after R16_UNORM quantization");
        std::printf("VR depth: range guards/idempotence passed; 64 GPU encode/decode/particle-fade cases passed; %d distinct nearby R16 depths\n",distinct);
        return 0;
    } catch(const std::exception& e) {std::fprintf(stderr,"%s\n",e.what());return 1;}
}
