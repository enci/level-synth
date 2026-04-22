#include "eval_engine.hpp"
#include "eval_context.hpp"
#include "node.hpp"
#include "node_graph.hpp"

#include <algorithm>
#include <functional>
#include <stdexcept>

namespace ls {

std::vector<int> eval_engine::topological_sort(const node_graph& graph) const {
    // Gather all node IDs
    std::vector<int> all_ids = graph.node_ids();

    // Build in-degree map
    std::unordered_map<int, int> in_degree;
    for (int id : all_ids)
        in_degree[id] = 0;

    for (const auto& w : graph.wires()) {
        if (in_degree.contains(w.to_node))
            in_degree[w.to_node]++;
    }

    // Kahn's algorithm
    std::vector<int> queue;
    for (auto [id, deg] : in_degree) {
        if (deg == 0) queue.push_back(id);
    }

    std::vector<int> order;
    order.reserve(all_ids.size());

    while (!queue.empty()) {
        int id = queue.back();
        queue.pop_back();
        order.push_back(id);

        for (const auto& w : graph.wires()) {
            if (w.from_node == id) {
                in_degree[w.to_node]--;
                if (in_degree[w.to_node] == 0)
                    queue.push_back(w.to_node);
            }
        }
    }

    if (order.size() != all_ids.size())
        throw std::runtime_error("Cycle detected in node graph");

    return order;
}

eval_context eval_engine::build_context(const node_graph& graph, int node_id, int master_seed) const {
    eval_context ctx;

    // Seed RNG: hash of master_seed and node_id
    std::size_t seed = std::hash<int>{}(master_seed) ^ (std::hash<int>{}(node_id) << 1);
    ctx.m_rng.seed(static_cast<std::mt19937::result_type>(seed));

    // Populate inputs from upstream cached outputs
    for (const auto& w : graph.wires()) {
        if (w.to_node != node_id) continue;

        auto cache_it = m_cache.find(w.from_node);
        if (cache_it == m_cache.end()) continue;

        auto output_it = cache_it->second.outputs.find(w.from_pin);
        if (output_it == cache_it->second.outputs.end()) continue;

        ctx.m_inputs[w.to_pin] = output_it->second;
    }

    return ctx;
}

void eval_engine::evaluate(node_graph& graph, int master_seed) {
    m_cache.clear();
    auto order = topological_sort(graph);

    for (int id : order) {
        // Skip if cached
        if (m_cache.contains(id)) continue;

        auto* n = graph.find_node(id);
        if (!n) continue;

        auto ctx = build_context(graph, id, master_seed);

        auto task = n->evaluate(ctx);
        if (!task) continue; // Evaluation failed, ski  p

        // Store outputs in cache
        m_cache[id].outputs = std::move(ctx.m_outputs);
    }
}

void eval_engine::invalidate_all() {
    m_cache.clear();
}

const pin_value* eval_engine::get_output(int node_id, const std::string& pin_name) const {
    auto cache_it = m_cache.find(node_id);
    if (cache_it == m_cache.end()) return nullptr;

    auto output_it = cache_it->second.outputs.find(pin_name);
    if (output_it == cache_it->second.outputs.end()) return nullptr;

    return &output_it->second;
}

}
