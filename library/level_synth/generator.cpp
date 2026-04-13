//
// Created by Bojan Endrovski on 13/04/2026.
//

#include "generator.hpp"
#include "eval_engine.hpp"
#include "attribute_grid.hpp"
#include "nodes/node_generator_input.hpp"
#include "nodes/node_generator_output.hpp"

namespace ls {

generator::generator() : m_engine(std::make_unique<eval_engine>()) {}
generator::~generator() = default;
generator::generator(const std::string& filepath) : m_engine(std::make_unique<eval_engine>()) {}

eval_engine& generator::engine() { return *m_engine; }
const eval_engine& generator::engine() const { return *m_engine; }

void generator::set_parameter(const std::string& name, double value) {}
void generator::set_seed(int seed) {}
void generator::evaluate() {}

std::shared_ptr<attribute_grid> generator::get_grid_output(const std::string& name) const { return nullptr; }
double generator::get_number_output(const std::string& name) const { return 0; }
void generator::rebuild_bindings() {}

} // namespace ls
