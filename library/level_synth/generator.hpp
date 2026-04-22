#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "node_graph.hpp"
#include "eval_engine.hpp"

namespace ls {

class attribute_grid;
class eval_engine;

class generator {
public:
    generator();
    ~generator();
    // explicit generator(const std::string& filepath);
    eval_engine& engine();
    node_graph& graph();
    void set_parameter(const std::string& name, double value);
    void set_seed(int seed);
    void evaluate();
    std::shared_ptr<attribute_grid> get_grid_output(const std::string& name) const;
    double get_number_output(const std::string& name) const;
    void rebuild_bindings();

private:
    node_graph m_graph;
    eval_engine m_engine;
    int m_seed = 0;
    std::unordered_map<std::string, int> m_param_nodes;
    std::unordered_map<std::string, int> m_output_nodes;
};

}
