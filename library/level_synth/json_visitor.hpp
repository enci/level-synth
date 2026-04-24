#pragma once

#include "node_visitor.hpp"

#include <nlohmann/json.hpp>


namespace ls {

/// Writes a node's persistent state into a JSON object.
///
/// Typical use:
///     nlohmann::json j;
///     json_writer w(j);
///     some_node.accept(w);
///     `j` now contains { "width": 64.0, "height": 64.0, ... }
class json_writer final : public node_visitor {
public:
    explicit json_writer(nlohmann::json& out) : m_out(out) {}

    void visit(std::string_view name, double& v) override;
    void visit(std::string_view name, int& v) override;
    void visit(std::string_view name, vec2& v) override;
    void visit(std::string_view name, std::string& v) override;
    // void visit(std::string_view name, bool& v) override;

    // `visit_custom` is intentionally not overridden — custom draws are a
    // UI concern, not a serialisation one. If a node has state that can't
    // be expressed as typed visits, it should expose it via dedicated
    // overloads (e.g. `visit(string_view, vector<rule>&)`) that serialisers
    // can implement explicitly.

private:
    nlohmann::json& m_out;
};


/// Reads a node's persistent state from a JSON object.
///
/// Missing keys leave the target unchanged, so defaults set at
/// construction time (e.g. `double m_width = 64;`) survive a load
/// from a JSON written by an older version of the node.
class json_reader final : public node_visitor {
public:
    explicit json_reader(const nlohmann::json& in) : m_in(in) {}

    void visit(std::string_view name, double& v) override;
    void visit(std::string_view name, int& v) override;
    void visit(std::string_view name, vec2& v) override;
    void visit(std::string_view name, std::string& v) override;
    // void visit(std::string_view name, bool& v) override;

private:
    const nlohmann::json& m_in;
};

}