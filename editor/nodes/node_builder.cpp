#define IMGUI_DEFINE_MATH_OPERATORS
#include "node_builder.hpp"
#include <imgui_internal.h>

namespace ed = ax::NodeEditor;

namespace editor {

void NodeBuilder::Begin(ed::NodeId id)
{
    HasHeader = false;
    HeaderMin = HeaderMax = ImVec2();

    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 4, 8, 8));
    ed::BeginNode(id);
    ImGui::PushID(id.AsPointer());
    CurrentNodeId = id;

    ImGui::BeginGroup();
}

void NodeBuilder::End()
{
    ImGui::EndGroup();
    ContentMin = ImGui::GetItemRectMin();

    ed::EndNode();

    // Draw colored header background on the node's background draw list
    if (HasHeader && ImGui::IsItemVisible())
    {
        auto alpha = static_cast<int>(255 * ImGui::GetStyle().Alpha);
        auto drawList = ed::GetNodeBackgroundDrawList(CurrentNodeId);
        const auto halfBorderWidth = ed::GetStyle().NodeBorderWidth * 0.5f;
        const auto rounding = ed::GetStyle().NodeRounding;

        auto headerColor = IM_COL32(0, 0, 0, alpha) | (HeaderColor & IM_COL32(255, 255, 255, 0));

        if (HeaderMax.x > HeaderMin.x && HeaderMax.y > HeaderMin.y)
        {
            drawList->AddRectFilled(
                HeaderMin - ImVec2(8 - halfBorderWidth, 4 - halfBorderWidth),
                HeaderMax + ImVec2(8 - halfBorderWidth, 0),
                headerColor, rounding, ImDrawFlags_RoundCornersTop);

            // Separator line below header
            drawList->AddLine(
                ImVec2(HeaderMin.x - (8 - halfBorderWidth), HeaderMax.y + 0.5f),
                ImVec2(HeaderMax.x + (8 - halfBorderWidth), HeaderMax.y + 0.5f),
                ImColor(255, 255, 255, 96 * alpha / (3 * 255)), 1.0f);
        }
    }

    CurrentNodeId = 0;
    ImGui::PopID();
    ed::PopStyleVar();
}

void NodeBuilder::Header(const ImVec4& color)
{
    HasHeader = true;
    HeaderColor = ImColor(color);
    ImGui::BeginGroup();
}

void NodeBuilder::EndHeader()
{
    ImGui::EndGroup();
    HeaderMin = ImGui::GetItemRectMin();
    HeaderMax = ImGui::GetItemRectMax();
    ImGui::Spacing();
}

void NodeBuilder::BeginColumns()
{
    ImGui::BeginGroup();
}

void NodeBuilder::NextColumn()
{
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
}

void NodeBuilder::EndColumns()
{
    ImGui::EndGroup();
}

} // namespace editor
