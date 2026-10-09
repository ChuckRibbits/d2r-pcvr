#include "../cleanroom/camera/deferred_draw.h"
#include <cassert>
#include <vector>

int main() {
    // Native traversal paints health before the tooltip updates its anchor.
    // Delay paint until that update; no prior eye/frame may leak into the next.
    int anchor=1800;
    std::vector<int> painted;
    for(const int eyeAnchor:{2430,1780,2450,1800}) {
        d2rcam::DeferredDraws pass;
        assert(pass.Add(1));assert(pass.Add(1));
        anchor=eyeAnchor;
        pass.Flush([&](uintptr_t widget){assert(widget==1);painted.push_back(anchor);});
        pass.Flush([&](uintptr_t){assert(false);});
    }
    assert((painted==std::vector<int>{2430,1780,2450,1800}));
    d2rcam::DeferredDraws bounded;
    for(uintptr_t i=1;i<=8;++i)assert(bounded.Add(i));
    assert(bounded.Add(1));assert(!bounded.Add(9));
    std::vector<uintptr_t> order;
    bounded.Flush([&](uintptr_t i){order.push_back(i);});
    assert((order==std::vector<uintptr_t>{1,2,3,4,5,6,7,8}));
    assert(bounded.Add(9));
}
