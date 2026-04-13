#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "pin.hpp"
#include "eval_task.hpp"
#include "eval_context.hpp"

namespace ls {

class node;

struct wire {
    int from_node;
    std::string from_pin;
    int to_node;
    std::string to_pin;
};

class eval_engine {
public:
    // --- Graph construction ---
    void add_node(std::unique_ptr<node> n);
    void remove_node(int node_id);
    void add_wire(const wire& w);
    void remove_wire(int from_node, const std::string& from_pin,
                     int to_node, const std::string& to_pin);

    node* find_node(int node_id);

    // --- Evaluation ---
    void evaluate(int master_seed = 0);

    // --- Results ---
    const pin_value* get_output(int node_id, const std::string& pin_name) const;

    // --- Stepping (editor debug) ---
    void begin_stepping(int master_seed = 0);
    bool step();
    int current_node_id() const;
    std::optional<eval_step> current_step() const;

private:
    std::vector<int> topological_sort() const;
    eval_context build_context(int node_id, int master_seed) const;

    std::unordered_map<int, std::unique_ptr<node>> m_nodes;
    std::vector<wire> m_wires;

    struct node_cache {
        std::unordered_map<std::string, pin_value> outputs;
    };
    std::unordered_map<int, node_cache> m_cache;

    // Stepping state
    std::vector<int> m_step_order;
    int m_step_index = -1;
    int m_step_seed = 0;
    std::optional<eval_context> m_step_ctx;  // kept alive while coroutine runs
    std::optional<eval_task> m_step_task;
};

}
