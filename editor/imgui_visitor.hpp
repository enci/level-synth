#pragma once

#include <level_synth/node_visitor.hpp>
#include <imgui.h>

/// A node_visitor that renders each field as an editable ImGui widget.
/// After accept(), check `changed` for live re-evaluate, `activated` to begin
/// an undo snapshot, and `deactivated_after_edit` to commit the snapshot.
class imgui_visitor : public ls::node_visitor {
public:
    bool changed               = false;
    bool activated             = false;
    bool deactivated_after_edit = false;

    void visit(std::string_view name, double& v) override {
        changed               |= ImGui::DragScalar(name.data(), ImGuiDataType_Double, &v,
                                                   0.1f, nullptr, nullptr, "%.3g");
        activated             |= ImGui::IsItemActivated();
        deactivated_after_edit |= ImGui::IsItemDeactivatedAfterEdit();
    }

    void visit(std::string_view name, int& v) override {
        changed               |= ImGui::DragInt(name.data(), &v);
        activated             |= ImGui::IsItemActivated();
        deactivated_after_edit |= ImGui::IsItemDeactivatedAfterEdit();
    }
};
