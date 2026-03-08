#pragma once

#include <imgui.h>

namespace editor {

// Custom color indices — set per theme, accessed via Colors[] array
enum Color_ {
    // Pin type colors (determines wire color in Blender fashion)
    Color_PinInteger,       // teal
    Color_PinReal,          // green
    Color_PinGrid,          // purple

    // Node header colors (by node category)
    Color_HeaderInput,      // green
    Color_HeaderProcess,    // orange
    Color_HeaderOutput,     // blue

    // Toolbar colors
    Color_ToolbarBg,
    Color_ToolbarBorder,
    Color_ToolbarButtonHovered,
    Color_ToolbarButtonActive,

    Color_COUNT
};

// Returns human-readable name for each color (for debug UI)
inline const char* GetColorName(Color_ idx) {
    switch (idx) {
        case Color_PinInteger:          return "Pin: Integer";
        case Color_PinReal:             return "Pin: Real";
        case Color_PinGrid:             return "Pin: Grid";
        case Color_HeaderInput:         return "Header: Input";
        case Color_HeaderProcess:       return "Header: Process";
        case Color_HeaderOutput:        return "Header: Output";
        case Color_ToolbarBg:           return "Toolbar: Background";
        case Color_ToolbarBorder:       return "Toolbar: Border";
        case Color_ToolbarButtonHovered: return "Toolbar: Button Hovered";
        case Color_ToolbarButtonActive: return "Toolbar: Button Active";
        default:                        return "Unknown";
    }
}

} // namespace editor
