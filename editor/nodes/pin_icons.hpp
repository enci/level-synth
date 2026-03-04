#pragma once

#include <imgui.h>

namespace editor {

enum class PinIconType : ImU32
{
    Circle,
    Square,
    Diamond
};

void DrawPinIcon(const ImVec2& size, PinIconType type, bool filled,
                 const ImVec4& color = ImVec4(1, 1, 1, 1),
                 const ImVec4& innerColor = ImVec4(0, 0, 0, 0));

} // namespace editor
