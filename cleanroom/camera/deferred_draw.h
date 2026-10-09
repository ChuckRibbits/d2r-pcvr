#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace d2rcam {
// One UI traversal only. Never retain widgets across frames or eye passes.
// Overflow is painted immediately by the caller, retaining the native fallback.
class DeferredDraws {
public:
    bool Add(uintptr_t widget) {
        for(size_t i=0;i<size_;++i)if(widgets_[i]==widget)return true;
        if(size_==widgets_.size())return false;
        widgets_[size_++]=widget;return true;
    }
    template<class Paint> void Flush(Paint paint) {
        const size_t count=size_;size_=0;
        for(size_t i=0;i<count;++i)paint(widgets_[i]);
    }
private:
    std::array<uintptr_t,8> widgets_{};
    size_t size_=0;
};
}
