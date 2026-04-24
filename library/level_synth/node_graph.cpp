#include "node_graph.hpp"
#include "json_visitor.hpp"
#include "node_registry.hpp"

namespace ls {

int node_graph::add_node(std::unique_ptr<node> n) {
    auto& reg = ls::node_registry::instance();
    int id = m_next_id++;
    n->m_id = id;
    if (n->m_name.empty()) {
        const auto* entry = reg.find(*n);
        if (entry)
            n->m_name = entry->display_name + " " + std::to_string(id);
    }
    m_nodes[id] = std::move(n);
    return id;
}

void node_graph::remove_node(int node_id) {
    m_nodes.erase(node_id);
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
    auto& reg = ls::node_registry::instance();

    nlohmann::json j;
    j["type"] = "level_synth_graph";
    j["version"] = 1.0;

    j["nodes"] = nlohmann::json::array();
    for (int id : node_ids()) {
        const node* n = find_node(id);
        if (!n) continue;

        const auto* entry = reg.find(*n);
        assert(entry && "Node not registered");

        nlohmann::json jn;
        jn["id"]   = n->id();
        jn["type"] = std::string(entry->type_name);
        json_writer writer(jn);
        const_cast<node*>(n)->accept(writer);

        j["nodes"].push_back(std::move(jn));
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

    return j.dump(2);
}

void node_graph::load(const std::string& data) {
    auto j = nlohmann::json::parse(data);

    clear();

    auto& reg = ls::node_registry::instance();

    for (const auto& jn : j.value("nodes", nlohmann::json::array())) {
        int saved_id     = jn["id"].get<int>();
        std::string type = jn["type"].get<std::string>();

        auto node_ptr = reg.create(type);
        if (!node_ptr) continue;

        json_reader reader(jn);
        node_ptr->accept(reader);

        // Restore the original id (accept() sets name and position but not id)
        node_ptr->m_id = saved_id;
        m_nodes[saved_id] = std::move(node_ptr);
        m_next_id = std::max(m_next_id, saved_id + 1);
    }

    for (const auto& jw : j.value("wires", nlohmann::json::array())) {
        m_wires.push_back({
            jw["from_node"].get<int>(),
            jw["from_pin"].get<std::string>(),
            jw["to_node"].get<int>(),
            jw["to_pin"].get<std::string>()
        });
    }
}

void node_graph::clear() {
    m_nodes.clear();
    m_wires.clear();
    m_next_id = 0;
}

}
