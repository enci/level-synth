#include "node_graph.hpp"
#include "json_visitor.hpp"

namespace ls {

int node_graph::add_node(std::unique_ptr<node> n) {
    int id = m_next_id++;
    n->m_id = id;
    if (n->m_name.empty())
        n->m_name = n->descriptor().name + " " + std::to_string(id);
    m_nodes[id] = std::move(n);
    return id;
}

void node_graph::remove_node(int node_id) {
    m_nodes.erase(node_id);

    // Remove any wires connected to this node
    std::erase_if(m_wires, [node_id](const wire& w) {
        return w.from_node == node_id || w.to_node == node_id;
    });
}

void node_graph::add_wire(const wire& w) {
    m_wires.push_back(w);
}

void node_graph::remove_wire(int from_node, const std::string& from_pin, int to_node, const std::string& to_pin) {
    std::erase_if(m_wires, [&](const wire& w) {
        return w.from_node == from_node && w.from_pin == from_pin && w.to_node == to_node && w.to_pin == to_pin;
    });
}

node* node_graph::find_node(int node_id) {
    auto it = m_nodes.find(node_id);
    return it != m_nodes.end() ? it->second.get() : nullptr;
}

    const node* node_graph::find_node(int node_id) const {
    auto it = m_nodes.find(node_id);
    return it != m_nodes.end() ? it->second.get() : nullptr;
}

std::vector<int> node_graph::node_ids() const {
    std::vector<int> ids;
    ids.reserve(m_nodes.size());
    for (const auto& [id, _] : m_nodes)
        ids.push_back(id);
    return ids;
}

const std::vector<wire>& node_graph::wires() const {
    return m_wires;
}

std::string node_graph::save() const {
    nlohmann::json j;
    j["version"] = 1;

    j["nodes"] = nlohmann::json::array();
    for (int id : node_ids()) {
        const node* n = find_node(id);
        if (!n) continue;

        nlohmann::json state;
        json_writer writer(state);
        const_cast<node*>(n)->accept(writer);

        j["nodes"].push_back({
            {"id",    n->id()},
            {"type",  std::string(n->descriptor().name)},
            {"name",  n->name()},
            {"state", std::move(state)},
        });
    }

    j["wires"] = nlohmann::json::array();
    for (const auto& w : m_wires) {
        j["wires"].push_back({
            {"from_node", w.from_node},
            {"from_pin",  w.from_pin},
            {"to_node",   w.to_node},
            {"to_pin",    w.to_pin},
        });
    }

    return j.dump(2);   // pretty-printed, 2-space indent
}

void node_graph::load(const std::string& json) {
    // nlohmann::json j = nlohmann::json::parse(json);
}

void node_graph::clear() {
    m_nodes.clear();
    m_wires.clear();
    m_next_id = 0;
}

}

