#pragma once

#include <level_synth/node_visitor.hpp>
#include <imgui.h>

/// A node_visitor that renders each field as an editable ImGui widget.
/// After accept(), check `changed` to know whether to re-evaluate the graph.
class imgui_visitor : public ls::node_visitor {
public:
    bool changed = false;

    void visit(std::string_view name, double& v) override {
        changed |= ImGui::DragScalar(name.data(), ImGuiDataType_Double, &v,
                                     0.1f, nullptr, nullptr, "%.3g");
    }

    void visit(std::string_view name, int& v) override {
        changed |= ImGui::DragInt(name.data(), &v);
    }
};
