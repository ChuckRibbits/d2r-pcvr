#pragma once

namespace framevr {
// A menu click must finish in the menu, even if that click closes the panel.
// Do not turn a still-held trigger into an attack on the next world frame.
struct MenuTriggerGate {
    bool consumed=false;
    bool Update(bool pressed,bool panel) {
        if(!pressed)consumed=false;
        else if(panel)consumed=true;
        return pressed&&!consumed;
    }
};

struct PressEdge {
    bool held=false;
    bool Update(bool pressed) {
        const bool edge=pressed&&!held;
        held=pressed;
        return edge;
    }
};

// Consume the entire click when used for calibration, including when the grip
// is released first. This prevents the same press from opening the map later.
struct HeightResetButton {
    bool wasClicked=false, consumed=false;
    bool Update(bool clicked, float grip) {
        const bool reset=clicked&&!wasClicked&&grip>.5f;
        if(!clicked)consumed=false;
        if(reset)consumed=true;
        wasClicked=clicked;
        return reset;
    }
    bool MapPressed() const {return wasClicked&&!consumed;}
};
}
