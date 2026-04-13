//
// Created by Bojan Endrovski on 13/04/2026.
//

#include "node_registry.hpp"
#include "node.hpp"

#include <stdexcept>

namespace ls {

void node_registry::register_node(const std::string& type_name, factory_fn factory) {}

std::unique_ptr<node> node_registry::create(const std::string& type_name) const { return nullptr; }

std::vector<std::string> node_registry::registered_types() const { return {}; }

const node_descriptor& node_registry::descriptor(const std::string& type_name) const {
    throw std::runtime_error("not implemented");
}

node_registry& node_registry::instance() {
    static node_registry reg;
    return reg;
}

} // namespace ls
