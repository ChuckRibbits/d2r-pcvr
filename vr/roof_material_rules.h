#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace roofmaterials {
constexpr uintptr_t BuildRva = 0xF99C40;
constexpr uintptr_t FindRva = 0x16153A0;
constexpr uint8_t BuildSig[] = {0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,
    0x48,0x89,0x74,0x24,0x18,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40};
constexpr uint8_t FindSig[] = {0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,
    0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x40};
// Six-argument material dispatch, including both stack arguments.
constexpr uint8_t BuildCallSig[] = {0x4c,0x8b,0xcf,0x89,0x44,0x24,0x28,0x41,0x8b,0x04,0x24,
    0x89,0x44,0x24,0x20,0xe8,0x84,0x84,0xff,0xff};
// Standard/VariantMaterial obtains the flag from this exact native member query.
constexpr uint8_t FlagCallSig[] = {0x49,0x8b,0x56,0x24,0x4c,0x8d,0x8d,0xf0,0x03,0,0,
    0x49,0x8b,0x4e,0x1c,0x4c,0x8d,0x05,0xf8,0xf0,0xe7,0,
    0x40,0x88,0x74,0x24,0x45,0xe8,0xa6,0xa4,0x67,0,
    0x84,0xc0,0x74,0x10,0x48,0x8b,0x85,0xf8,0x03,0,0,0x0f,0xb6,0x00};
// GrannyFindMember publishes (member definition, member value), not a copy.
constexpr uint8_t FindResultSig[] = {0x48,0x85,0xff,0x74,0x07,0x48,0x89,0x1f,
    0x48,0x89,0x77,0x08,0xb0,0x01};

template<class Check> bool Verified(Check check) {
    return check(BuildRva,BuildSig,sizeof BuildSig) && check(FindRva,FindSig,sizeof FindSig) &&
        check(0xFA17A8,BuildCallSig,sizeof BuildCallSig) &&
        check(0xF9AEDA,FlagCallSig,sizeof FlagCallSig) &&
        check(0x16317C3,FindResultSig,sizeof FindResultSig);
}

inline bool RoofToken(std::string_view s) {
    for (size_t at=s.find("roof"); at!=s.npos; at=s.find("roof",at+4)) {
        const size_t end=at+4;
        if ((at==0 || s[at-1]=='_' || s[at-1]=='/') &&
            (end==s.size() || s[end]=='_' || s[end]=='/' || (s[end]>='0'&&s[end]<='9'))) return true;
    }
    return false;
}
// Inputs have already been bounded and canonicalized by the guarded reader.
inline bool Selected(std::string_view model, std::string_view material) {
    constexpr std::string_view prefix="data/hd/env/model/act";
    if (!model.starts_with(prefix) || model.size()<=prefix.size()+2 ||
        model[prefix.size()]<'1' || model[prefix.size()]>'5' || model[prefix.size()+1]!='/' ||
        model.find("..")!=model.npos || !model.ends_with(".model")) return false;
    constexpr std::string_view tavern="data/hd/env/model/act2/town/act2_town_building_tavern/";
    if (model.starts_with(tavern)) {
        const auto file=model.substr(tavern.size());
        // The lower and upper storeys share the thin exterior shell. Keep
        // coverage consistent as either switches LOD; never include props.
        const bool level01=file.starts_with("level01"),level02=file.starts_with("level02");
        if (!level01&&!level02) return false;
        const auto suffix=file.substr(7);
        if (suffix!=".model" && !(suffix.size()==11 && suffix.starts_with("_lod") &&
            suffix[4]>='0' && suffix[4]<='4' && suffix.substr(5)==".model")) return false;
        // Audited native materials: roof/floor surfaces above the player,
        // plus the wall panels, arches and window surrounds visible from
        // inside. IsDoubleSided does not fill modeled window/door openings.
        constexpr std::string_view shell[]={
            "ext_half_circle_roof","ext_roof_tile01","ext_roof_tile02",
            "ext_twig_roof","ext_roof_trim","ext_awning01","int_stockroom_roof",
            "ext_wall01a","ext_half_circle_base","ext_rear_arch",
            "ext_front_entrance","ext_awning_shutters",
            "ext_front_window01_01","ext_front_window01_02",
            "ext_front_window02_01","ext_front_window02_02","ext_side_window01",
            "ext_overhang_window01a_01","ext_overhang_window01a_02",
            "ext_overhang_window01b_01","ext_overhang_window01b_02",
            "ext_overhang_window02_01","ext_overhang_window02_02",
            "int_plaster01a","int_plaster01b","int_plaster02a","int_plaster02b","int_dividers"
        };
        if (level02&&material=="ext_floor") return true; // ceiling from below
        for (const auto name:shell) if (material==name) return true;
        return false;
    }
    // Dedicated environment roof families; never match a material name alone.
    auto file=model.substr(model.find_last_of('/')+1);
    file.remove_suffix(6);
    return RoofToken(file) && RoofToken(material);
}

struct Context { const void* type=nullptr; const void* object=nullptr; unsigned changes=0; };
struct Scope {
    Context*& slot; Context* previous;
    Scope(Context*& slot_,Context* next):slot(slot_),previous(slot_){slot=next;}
    ~Scope(){slot=previous;}
    Scope(const Scope&)=delete; Scope& operator=(const Scope&)=delete;
};
inline bool Override(const Context* ctx,const void* type,const void* object,
                     std::string_view name,int kind,int width,int value) {
    return ctx && ctx->type && ctx->object && ctx->type==type && ctx->object==object &&
        name=="IsDoubleSided" && (kind==19||kind==20) && (width==0||width==1) && value==0;
}
}
