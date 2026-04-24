#include "node_registry.hpp"
#include "node.hpp"

#include <stdexcept>
#include <cassert>

namespace ls {

node_registry& node_registry::instance() {
    static node_registry reg;
    return reg;
}

void node_registry::register_node(node_registration&& entry) {
    auto [it, inserted] = m_entries.emplace(entry.type_name, std::move(entry));
    assert(inserted && "Duplicate node type registration");
}

std::unique_ptr<node> node_registry::create(const std::string& type_name) const {
    auto it = m_entries.find(type_name);
    if (it == m_entries.end())
        throw std::runtime_error("Unknown node type: " + type_name);
    return it->second.factory();
}

const node_registration* node_registry::find(const node& n) const {
    const auto idx = std::type_index(typeid(n));
    for (const auto& [_, entry] : m_entries) {
        if (entry.type_idx == idx) return &entry;
    }
    return nullptr;
}

}
