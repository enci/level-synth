#pragma once

#include <imgui.h>

namespace editor {

enum Color_ {
    // Pin type colors (determines wire color)
    Color_PinNumber,        // teal
    Color_PinGrid,          // purple

    // Node header colors (by node category)
    Color_HeaderInput,      // green
    Color_HeaderProcess,    // orange
    Color_HeaderOutput,     // blue

    // Status bar colors
    Color_StatusBarBg,
    Color_StatusBarBorder,

    Color_COUNT
};

inline const char* GetColorName(Color_ idx) {
    switch (idx) {
    case Color_PinNumber:           return "Pin: Number";
    case Color_PinGrid:             return "Pin: Grid";
    case Color_HeaderInput:         return "Header: Input";
    case Color_HeaderProcess:       return "Header: Process";
    case Color_HeaderOutput:        return "Header: Output";
    case Color_StatusBarBg:          return "Status Bar: Background";
    case Color_StatusBarBorder:      return "Status Bar: Border";
    default:                        return "Unknown";
    }
}

}