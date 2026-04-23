#pragma once

#include <string_view>
#include <string>
// #include <functional>

namespace ls {

// class grid;

/// Base interface for operations that enumerate a node's persistent state.
///
/// Nodes implement `accept(node_visitor&)` by calling `visit()` once per
/// persistent member. Concrete visitors (serialisation, UI, schema, …)
/// override only the overloads for types they care about; the rest do
/// nothing by default.
///
/// All overloads take non-const references so the same interface can
/// both read and write node state — a reader visitor pulls values from
/// its source and assigns them; a writer visitor reads them out.
class node_visitor {
public:
    virtual ~node_visitor() = default;
    virtual void visit(std::string_view name, double& v) {}
    virtual void visit(std::string_view name, int& v) {}
    // virtual void visit(std::string_view name, bool& v) {}
    // virtual void visit(std::string_view name, std::string& v) {}

    // Extend with additional types as they appear in nodes.
    // Examples you'll likely want later:
    // virtual void visit(std::string_view name, std::shared_ptr<grid>& v) {}
    // virtual void visit(std::string_view name, std::vector<double>& v) {}

    /// Hook for node-specific UI (and anything else that can't be expressed
    /// as a sequence of typed visits). The node passes a draw callback; most
    /// visitors ignore it. The UI visitor invokes it to let the node render
    /// bespoke widgets (custom grid painter, rule-list editor, …).
    // virtual void visit_custom(std::string_view name, const std::function<void()>& draw) {}
};

} // namespace ls