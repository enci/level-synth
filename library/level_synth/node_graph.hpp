#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include "node.hpp"
#include "pin.hpp"
#include <nlohmann/json.hpp>

namespace ls {

/// A wire in the graph connecting an output pin of one node to an input pin of another node.
struct wire {
    int from_node;
    std::string from_pin;
    int to_node;
    std::string to_pin;
};

class node_graph {
public:
    /// Add a node to the graph. The node's ID will be set automatically. Returns the assigned ID.
    int add_node(std::unique_ptr<node> n);

    /// Remove a node by id
    void remove_node(int node_id);

    /// Add a new wire (wire value is copied)
    /// The destination node and all downstream nodes will be invalidated.
    void add_wire(const wire& w);

    /// Remove a wire
    void remove_wire(int from_node, const std::string& from_pin,
                     int to_node, const std::string& to_pin);

    // TODO: Maybe add an option to remove by value

    /// Find a node by id. Returns nullptr if not found.
    node* find_node(int node_id);

    /// Find a node by id. Returns nullptr if not found.
    const node* find_node(int node_id) const;

    /// Get all the nodes (used for the ui)
    std::vector<int> node_ids() const;

    /// Get all the wires (used for the ui)
    const std::vector<wire>& wires() const;

    /// Save as json
    std::string save() const;

    /// Load from json (existing graph will be cleared)
    void load(const std::string& data);

    /// Clear the reg
    void clear();

private:
    friend class eval_engine;

    std::unordered_map<int, std::unique_ptr<node>> m_nodes;
    std::vector<wire> m_wires;
    int m_next_id = 0;
};

}
