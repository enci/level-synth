#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "pin.hpp"
#include "eval_context.hpp"
#include "node_graph.hpp"

namespace ls {

class node;

class eval_engine {
public:
    void evaluate(node_graph& graph, int master_seed = 0);
    const pin_value* get_output(int node_id, const std::string& pin_name) const;
    void invalidate_all();

private:
    std::vector<int> topological_sort(const node_graph& graph) const;
    eval_context build_context(const node_graph& graph, int node_id, int master_seed) const;

    struct node_cache {
        std::unordered_map<std::string, pin_value> outputs;
    };

    std::unordered_map<int, node_cache> m_cache;
};

}