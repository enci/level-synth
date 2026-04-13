//
// Created by Bojan Endrovski on 13/04/2026.
//

#include "eval_engine.hpp"
#include "node.hpp"
#include "eval_context.hpp"

namespace ls {

void eval_engine::add_node(std::unique_ptr<node> n) {}
void eval_engine::remove_node(int node_id) {}
void eval_engine::add_wire(const wire& w) {}
void eval_engine::remove_wire(int from_node, const std::string& from_pin,
                              int to_node, const std::string& to_pin) {}

node* eval_engine::find_node(int node_id) { return nullptr; }

void eval_engine::evaluate(int master_seed) {}

const pin_value* eval_engine::get_output(int node_id, const std::string& pin_name) const { return nullptr; }

void eval_engine::begin_stepping(int master_seed) {}
bool eval_engine::step() { return false; }
int eval_engine::current_node_id() const { return 0; }
std::optional<eval_step> eval_engine::current_step() const { return std::nullopt; }

std::vector<int> eval_engine::topological_sort() const { return {}; }
eval_context eval_engine::build_context(int node_id, int master_seed) const { return {}; }

} // namespace ls
