#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
namespace framevr {
// Thread-local copied values only. A generation change invalidates positive
// AND negative lookups, including destroyed/recreated native handle values.
template<class Value,size_t Size=64> class GenerationCache {
    static_assert(Size>0&&(Size&(Size-1))==0);
    struct Entry {uint64_t key=0,generation=0;Value value{};bool valid=false;};
    std::array<Entry,Size> entries{};
    Entry& At(uint64_t key){return entries[((key>>4)^(key>>12)^key)&(Size-1)];}
public:
    bool Get(uint64_t key,uint64_t generation,Value& value) {
        const auto& e=At(key);
        if(!e.valid||e.key!=key||e.generation!=generation)return false;
        value=e.value;return true;
    }
    void Put(uint64_t key,uint64_t generation,Value value){At(key)={key,generation,value,true};}
};
}
