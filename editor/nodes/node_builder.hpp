#pragma once

#include <imgui_node_editor.h>

namespace editor {

// Simplified node builder using standard ImGui layout (BeginGroup/EndGroup).
// Draws Blender-style colored header rectangles on nodes.
struct NodeBuilder
{
    void Begin(ax::NodeEditor::NodeId id);
    void End();

    void Header(const ImVec4& color = ImVec4(1, 1, 1, 1));
    void EndHeader();

    // Call between Header/EndHeader or after EndHeader for inputs column
    void BeginColumns();
    void NextColumn();
    void EndColumns();

private:
    ax::NodeEditor::NodeId CurrentNodeId = 0;
    ImU32       HeaderColor = IM_COL32(255, 255, 255, 255);
    ImVec2      HeaderMin;
    ImVec2      HeaderMax;
    ImVec2      ContentMin;
    bool        HasHeader = false;
};

} // namespace editor
