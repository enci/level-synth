#include "node_registry.hpp"
#include "node.hpp"

#include <stdexcept>

namespace ls {

node_registry& node_registry::instance() {
    static node_registry reg;
    return reg;
}

void node_registry::register_node(const std::string& type_name, factory_fn factory) {
    m_entries[type_name] = entry{ std::move(factory), nullptr };
}

std::unique_ptr<node> node_registry::create(const std::string& type_name) const {
    auto it = m_entries.find(type_name);
    if (it == m_entries.end())
        throw std::runtime_error("Unknown node type: " + type_name);
    return it->second.factory();
}

std::vector<std::string> node_registry::registered_types() const {
    std::vector<std::string> types;
    types.reserve(m_entries.size());
    for (const auto& [name, _] : m_entries)
        types.push_back(name);
    return types;
}

const node_descriptor& node_registry::descriptor(const std::string& type_name) const {
    auto it = m_entries.find(type_name);
    if (it == m_entries.end())
        throw std::runtime_error("Unknown node type: " + type_name);

    // Lazy-create a prototype instance to access its descriptor
    if (!it->second.prototype)
        it->second.prototype = it->second.factory();

    return it->second.prototype->descriptor();
}

}