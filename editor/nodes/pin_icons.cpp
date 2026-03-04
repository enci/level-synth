#define IMGUI_DEFINE_MATH_OPERATORS
#include "pin_icons.hpp"
#include <imgui_internal.h>

namespace editor {

void DrawPinIcon(const ImVec2& size, PinIconType type, bool filled,
                 const ImVec4& color, const ImVec4& innerColor)
{
    if (ImGui::IsRectVisible(size))
    {
        auto cursorPos = ImGui::GetCursorScreenPos();
        auto drawList  = ImGui::GetWindowDrawList();

        auto rect = ImRect(cursorPos, cursorPos + size);
        auto rect_center = ImVec2(
            (rect.Min.x + rect.Max.x) * 0.5f,
            (rect.Min.y + rect.Max.y) * 0.5f);
        auto rect_w = rect.Max.x - rect.Min.x;

        const auto outline_scale = rect_w / 24.0f;
        const auto extra_segments = static_cast<int>(2 * outline_scale);

        auto col = ImColor(color);
        auto innerCol = ImColor(innerColor);

        if (type == PinIconType::Circle)
        {
            if (filled)
            {
                drawList->AddCircleFilled(rect_center, 0.5f * rect_w / 2.0f,
                    col, 12 + extra_segments);
            }
            else
            {
                const auto r = 0.5f * rect_w / 2.0f - 0.5f;
                if (innerCol & 0xFF000000)
                    drawList->AddCircleFilled(rect_center, r, innerCol, 12 + extra_segments);
                drawList->AddCircle(rect_center, r, col, 12 + extra_segments,
                    2.0f * outline_scale);
            }
        }
        else if (type == PinIconType::Square)
        {
            const auto r = 0.5f * rect_w / 2.0f;
            const auto cr = r * 0.4f;
            const auto p0 = rect_center - ImVec2(r, r);
            const auto p1 = rect_center + ImVec2(r, r);

            if (filled)
            {
                drawList->AddRectFilled(p0, p1, col, cr, ImDrawFlags_RoundCornersAll);
            }
            else
            {
                if (innerCol & 0xFF000000)
                    drawList->AddRectFilled(p0, p1, innerCol, cr, ImDrawFlags_RoundCornersAll);
                drawList->AddRect(p0, p1, col, cr, ImDrawFlags_RoundCornersAll,
                    2.0f * outline_scale);
            }
        }
        else if (type == PinIconType::Diamond)
        {
            const auto r = 0.607f * rect_w / 2.0f;

            if (filled)
            {
                drawList->PathLineTo(rect_center + ImVec2( 0, -r));
                drawList->PathLineTo(rect_center + ImVec2( r,  0));
                drawList->PathLineTo(rect_center + ImVec2( 0,  r));
                drawList->PathLineTo(rect_center + ImVec2(-r,  0));
                drawList->PathFillConvex(col);
            }
            else
            {
                const auto r2 = r - 0.5f;
                drawList->PathLineTo(rect_center + ImVec2( 0, -r2));
                drawList->PathLineTo(rect_center + ImVec2( r2,  0));
                drawList->PathLineTo(rect_center + ImVec2( 0,  r2));
                drawList->PathLineTo(rect_center + ImVec2(-r2,  0));

                if (innerCol & 0xFF000000)
                    drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, innerCol);

                drawList->PathStroke(col, true, 2.0f * outline_scale);
            }
        }
    }

    ImGui::Dummy(size);
}

} // namespace editor
