#pragma once
#include <cstdint>

namespace framevr::interaction {
// Native controller UI exposes a single Town Portal shortcut for scroll/book;
// Identify is used through items, not bound as the raw scroll/book skill.
constexpr int CatalogSkillId(int id){return id==217||id==218?-1:id==219||id==220?359:id;}
// Native controller records: two weapon-set banks, 16 records per bank.
// The generic native getter chooses keyboard vs controller by last input mode;
// a VR wheel must always read the controller table, without changing that mode.
struct ControllerBinding {
    int32_t skill=-1,owner=-1;
    uint32_t reserved[5]{};
};
static_assert(sizeof(ControllerBinding)==28);
template<class Read> bool ReadControllerBinding(uintptr_t table,int bank,int slot,Read read,ControllerBinding& out) {
    out={};
    if(!table||(bank!=0&&bank!=16)||slot<0||slot>=16)return false;
    ControllerBinding value;
    if(!read(table+uintptr_t(bank+slot)*sizeof(value),value)||value.skill<-1||value.skill>4095)return false;
    out=value;return true;
}
}
