#include "node_graph.hpp"

namespace ls {

int node_graph::add_node(std::unique_ptr<node> n) {
    int id = m_next_id++;
    n->m_id = id;
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

}

