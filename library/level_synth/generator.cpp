#include "generator.hpp"
#include "eval_engine.hpp"
#include "nodes/node_input_number.hpp"
#include "nodes/node_output_grid.hpp"
#include "nodes/node_output_number.hpp"

#include <stdexcept>

namespace ls {

generator::generator() {}

generator::~generator() = default;

/*
generator::generator(const std::string& filepath)
    : m_engine(std::make_unique<eval_engine>()) {
    // TODO: load graph from file
}
*/

void generator::set_parameter(const std::string& name, double value) {
    auto it = m_param_nodes.find(name);
    if (it == m_param_nodes.end())
        throw std::runtime_error("Unknown parameter: " + name);

    auto* n = dynamic_cast<node_input_number*>(m_graph.find_node(it->second));
    if (!n) return;
}

void generator::set_seed(int seed) {
    m_seed = seed;
}

void generator::evaluate() {
    m_engine.evaluate(m_graph, m_seed);
}

std::shared_ptr<grid> generator::get_grid_output(const std::string& name) const {
    auto it = m_output_nodes.find(name);
    if (it == m_output_nodes.end())
        throw std::runtime_error("Unknown output: " + name);

    auto* val = m_engine.get_output(it->second, "value");
    if (!val) return nullptr;

    return std::get<std::shared_ptr<grid>>(*val);
}

double generator::get_number_output(const std::string& name) const {
    auto it = m_output_nodes.find(name);
    if (it == m_output_nodes.end())
        throw std::runtime_error("Unknown output: " + name);

    auto* val = m_engine.get_output(it->second, "value");
    if (!val) return 0;

    return std::get<double>(*val);
}

void generator::rebuild_bindings() {
    m_param_nodes.clear();
    m_output_nodes.clear();

    for (int id : m_graph.node_ids()) {
        auto* n = m_graph.find_node(id);
        if (!n) continue;

        if (auto* input = dynamic_cast<node_input_number*>(n)) {
            m_param_nodes[input->name()] = id;
        } else if (auto* output = dynamic_cast<node_output_grid*>(n)) {
            m_output_nodes[output->name()] = id;
        } else if (auto* output = dynamic_cast<node_output_number*>(n)) {
            m_output_nodes[output->name()] = id;
        }
    }
}

} // namespace ls
