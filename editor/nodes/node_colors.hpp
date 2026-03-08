#pragma once

#include <imgui.h>

namespace editor {

// ---- Pin type colors (determines wire color in Blender fashion) ----
namespace pin_color {
    constexpr ImVec4 integer = ImVec4(0.25f, 0.75f, 0.85f, 1.0f);  // teal
    constexpr ImVec4 real    = ImVec4(0.45f, 0.78f, 0.45f, 1.0f);  // green
    constexpr ImVec4 grid    = ImVec4(0.65f, 0.40f, 0.85f, 1.0f);  // purple
}

// ---- Node header colors (by node category) ----
namespace header_color {
    constexpr ImVec4 input   = ImVec4(0.30f, 0.60f, 0.30f, 1.0f);  // green
    constexpr ImVec4 process = ImVec4(0.75f, 0.45f, 0.20f, 1.0f);  // orange
    constexpr ImVec4 output  = ImVec4(0.30f, 0.45f, 0.70f, 1.0f);  // blue
}

} // namespace editor
