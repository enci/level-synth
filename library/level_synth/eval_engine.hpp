#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "pin.hpp"
#include "eval_context.hpp"

namespace ls {

class node;
class node_graph;

struct wire {
    int from_node;
    std::string from_pin;
    int to_node;
    std::string to_pin;
};

class eval_engine {
public:
    void evaluate(node_graph& graph, int master_seed = 0);
    const pin_value* get_output(int node_id, const std::string& pin_name) const;
    void invalidate_all();

private:
    std::vector<int> topological_sort() const;
    eval_context build_context(int node_id, int master_seed) const;

    //int m_next_id = 1;
    //std::unordered_map<int, std::unique_ptr<node>> m_nodes;
    //std::vector<wire> m_wires;

   //struct node_cache {
   //    std::unordered_map<std::string, pin_value> outputs;
   //};

   // std::unordered_map<int, node_cache> m_cache;
    // Stepping state
    //std::vector<int> m_step_order;
    //int m_step_index = -1;
    //int m_step_seed = 0;
};

}