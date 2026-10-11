#include "composition_layers.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace framevr::composition;
void Check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
// Reproduce the observed bridge allocator, including its incorrect reuse of
// the first element's size. Conversion preserves fields except native handles.
bool SurvivesBridge(const XrCompositionLayerBaseHeader* const* layers,uint32_t count) {
    const auto size=[](XrStructureType t){return t==XR_TYPE_COMPOSITION_LAYER_QUAD?sizeof(XrCompositionLayerQuad):sizeof(XrCompositionLayerProjection);};
    const size_t stride=size(layers[0]->type);
    std::vector<unsigned char> arena(count*stride+sizeof(XrCompositionLayerQuad));
    for(uint32_t i=0;i<count;++i)std::memcpy(arena.data()+i*stride,layers[i],size(layers[i]->type));
    for(uint32_t i=0;i<count;++i)if(std::memcmp(arena.data()+i*stride,layers[i],size(layers[i]->type)))return false;
    return true;
}
int main(){
    XrCompositionLayerProjection scene{XR_TYPE_COMPOSITION_LAYER_PROJECTION};scene.viewCount=2;
    XrCompositionLayerQuad menu{XR_TYPE_COMPOSITION_LAYER_QUAD},laser{XR_TYPE_COMPOSITION_LAYER_QUAD},padding{};
    menu.pose.orientation.w=laser.pose.orientation.w=1;menu.size={.95f,.76f};laser.size={.003f,1.5f};
    menu.subImage.imageRect={{0,0},{1280,1024}};laser.subImage.imageRect={{0,0},{64,32}};
    const auto base=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&scene);
    const auto atlas=reinterpret_cast<XrSwapchain>(uintptr_t(0x1000));
    const auto space=reinterpret_cast<XrSpace>(uintptr_t(0x2000));
    std::array<const XrCompositionLayerBaseHeader*,4> layers{base,
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&menu),
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&laser)};
    Check(!SurvivesBridge(layers.data(),3),"reproduces overlapping quad corruption");
    Check(NeedsQuadPadding(true,base,atlas),"mixed Wine frame needs padding");
    Check(!NeedsQuadPadding(false,base,atlas),"native Windows unchanged");
    Check(!NeedsQuadPadding(true,layers[1],atlas),"flat menu already sizes quads correctly");
    uint32_t count=3;
    Check(PrependQuadPadding(layers.data(),count,layers.size(),padding,space,atlas),"prefix added");
    Check(count==4&&layers[1]==base&&layers[2]==reinterpret_cast<const XrCompositionLayerBaseHeader*>(&menu),"scene and overlay order retained");
    Check(SurvivesBridge(layers.data(),count),"mixed frame survives actual bridge allocation rule");
    Check(padding.pose.orientation.w==1&&padding.subImage.imageRect.offset.y==32&&padding.subImage.imageRect.extent.width==1,"valid transparent prefix");
    Check(!PrependQuadPadding(layers.data(),count,layers.size(),padding,space,atlas),"capacity respected");
    count=1;layers[0]=base;
    Check(!PrependQuadPadding(layers.data(),count,layers.size(),padding,space,atlas)&&count==1,"scene-only frame adds nothing");
    std::puts("Composition: reproduced Wine quad overlap; padding preserves fields, order, capacity and native path");
}
