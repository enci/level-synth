#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace ls {

class attribute_grid;
class eval_engine;

class generator {
public:
    generator();
    ~generator();

    explicit generator(const std::string& filepath);

    // --- Graph building (editor uses this) ---
    eval_engine& engine();
    const eval_engine& engine() const;

    // --- Runtime API (game uses this) ---
    void set_parameter(const std::string& name, double value);

    void set_seed(int seed);
    void evaluate();

    // Retrieve named outputs (from Generator Output nodes)
    std::shared_ptr<attribute_grid> get_grid_output(const std::string& name) const;
    double get_number_output(const std::string& name) const;

    // Scans the graph for Generator Input/Output nodes
    // and rebuilds the parameter and output binding maps.
    void rebuild_bindings();

private:
    std::unique_ptr<eval_engine> m_engine;
    int m_seed = 0;

    std::unordered_map<std::string, int> m_param_nodes;
    std::unordered_map<std::string, int> m_output_nodes;
};

}