#include "eval_engine.hpp"
#include "eval_context.hpp"
#include "node.hpp"

#include <algorithm>
#include <functional>
#include <stdexcept>

namespace ls {

    /*
int eval_engine::add_node(std::unique_ptr<node> n) {
    int id = m_next_id++;
    n->m_id = id;
    m_nodes[id] = std::move(n);
    return id;
}

void eval_engine::remove_node(int node_id) {
    m_nodes.erase(node_id);
    m_cache.erase(node_id);

    // Remove any wires connected to this node
    std::erase_if(m_wires, [node_id](const wire& w) {
        return w.from_node == node_id || w.to_node == node_id;
    });
}

void eval_engine::add_wire(const wire& w) {
    m_wires.push_back(w);

    // Invalidate the destination node and everything downstream
    invalidate(w.to_node);
}

void eval_engine::remove_wire(int from_node, const std::string& from_pin,
                               int to_node, const std::string& to_pin) {
    std::erase_if(m_wires, [&](const wire& w) {
        return w.from_node == from_node && w.from_pin == from_pin
            && w.to_node == to_node && w.to_pin == to_pin;
    });

    invalidate(to_node);
}

node* eval_engine::find_node(int node_id) {
    auto it = m_nodes.find(node_id);
    return it != m_nodes.end() ? it->second.get() : nullptr;
}

const node* eval_engine::find_node(int node_id) const {
    auto it = m_nodes.find(node_id);
    return it != m_nodes.end() ? it->second.get() : nullptr;
}

std::vector<int> eval_engine::node_ids() const {
    std::vector<int> ids;
    ids.reserve(m_nodes.size());
    for (const auto& [id, _] : m_nodes)
        ids.push_back(id);
    return ids;
}

// --- Invalidation ---

void eval_engine::invalidate(int node_id) {
    m_cache.erase(node_id);

    // Invalidate all downstream nodes recursively
    for (const auto& w : m_wires) {
        if (w.from_node == node_id) {
            invalidate(w.to_node);
        }
    }
}

// --- Topological sort ---

std::vector<int> eval_engine::topological_sort() const {
    // Gather all node IDs
    std::vector<int> all_ids;
    all_ids.reserve(m_nodes.size());
    for (const auto& [id, _] : m_nodes)
        all_ids.push_back(id);

    // Build in-degree map
    std::unordered_map<int, int> in_degree;
    for (int id : all_ids)
        in_degree[id] = 0;

    for (const auto& w : m_wires) {
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

        for (const auto& w : m_wires) {
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

// --- Context building ---

eval_context eval_engine::build_context(int node_id, int master_seed) const {
    eval_context ctx;

    // Seed RNG: hash of master_seed and node_id
    std::size_t seed = std::hash<int>{}(master_seed) ^ (std::hash<int>{}(node_id) << 1);
    ctx.m_rng.seed(static_cast<std::mt19937::result_type>(seed));

    // Populate inputs from upstream cached outputs
    for (const auto& w : m_wires) {
        if (w.to_node != node_id) continue;

        auto cache_it = m_cache.find(w.from_node);
        if (cache_it == m_cache.end()) continue;

        auto output_it = cache_it->second.outputs.find(w.from_pin);
        if (output_it == cache_it->second.outputs.end()) continue;

        ctx.m_inputs[w.to_pin] = output_it->second;
    }

    return ctx;
}

// --- Full evaluation ---

void eval_engine::evaluate(int master_seed) {
    auto order = topological_sort();

    for (int id : order) {
        // Skip if cached
        if (m_cache.contains(id)) continue;

        auto* n = find_node(id);
        if (!n) continue;

        auto ctx = build_context(id, master_seed);

        auto task = n->evaluate(ctx);
        task.run();

        // Store outputs in cache
        m_cache[id].outputs = std::move(ctx.m_outputs);
    }
}

// --- Results ---

const pin_value* eval_engine::get_output(int node_id, const std::string& pin_name) const {
    auto cache_it = m_cache.find(node_id);
    if (cache_it == m_cache.end()) return nullptr;

    auto output_it = cache_it->second.outputs.find(pin_name);
    if (output_it == cache_it->second.outputs.end()) return nullptr;

    return &output_it->second;
}

// --- Stepping ---

void eval_engine::begin_stepping(int master_seed) {
    m_step_order = topological_sort();
    m_step_index = 0;
    m_step_seed = master_seed;
    m_step_ctx.reset();
    m_step_task.reset();
}

bool eval_engine::step() {
    // If there's an active coroutine that hasn't finished, resume it
    if (m_step_task && !m_step_task->done()) {
        m_step_task->resume();

        // If the coroutine just finished, store outputs
        if (m_step_task->done()) {
            int id = m_step_order[m_step_index];
            m_cache[id].outputs = std::move(m_step_ctx->m_outputs);
            m_step_ctx.reset();
            m_step_task.reset();
            m_step_index++;
        }
        return m_step_index < static_cast<int>(m_step_order.size());
    }

    // No active coroutine — start the next node
    if (m_step_index >= static_cast<int>(m_step_order.size()))
        return false;

    int id = m_step_order[m_step_index];

    // Skip if cached
    if (m_cache.contains(id)) {
        m_step_index++;
        return m_step_index < static_cast<int>(m_step_order.size());
    }

    auto* n = find_node(id);
    if (!n) {
        m_step_index++;
        return m_step_index < static_cast<int>(m_step_order.size());
    }

    m_step_ctx = build_context(id, m_step_seed);
    m_step_task = n->evaluate(*m_step_ctx);

    // If the coroutine completed immediately (no yields), store and advance
    if (m_step_task->done()) {
        m_cache[id].outputs = std::move(m_step_ctx->m_outputs);
        m_step_ctx.reset();
        m_step_task.reset();
        m_step_index++;
    }

    return m_step_index < static_cast<int>(m_step_order.size());
}

int eval_engine::current_node_id() const {
    if (m_step_index >= 0 && m_step_index < static_cast<int>(m_step_order.size()))
        return m_step_order[m_step_index];
    return -1;
}

std::optional<eval_step> eval_engine::current_step() const {
    if (m_step_task)
        return m_step_task->current_step();
    return std::nullopt;
}
*/

}
