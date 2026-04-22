#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include "node.hpp"
#include "pin.hpp"

namespace ls {

/// A wire in the graph connecting an output pin of one node to an input pin of another node.
struct wire {
    int from_node;
    std::string from_pin;
    int to_node;
    std::string to_pin;
};

///
class node_graph {
public:
    /// Add a node to the graph. The node's ID will be set automatically. Returns the assigned ID.
    int add_node(std::unique_ptr<node> n);
    void remove_node(int node_id);
    void add_wire(const wire& w);
    void remove_wire(int from_node, const std::string& from_pin,
                     int to_node, const std::string& to_pin);
    // TODO: Maybe add an option to remove by value

    node* find_node(int node_id);
    const node* find_node(int node_id) const;
    std::vector<int> node_ids() const;
    const std::vector<wire>& wires() const;

private:
    friend class eval_engine;
    std::unordered_map<int, std::unique_ptr<node>> m_nodes;
    std::vector<wire> m_wires;
};

}
