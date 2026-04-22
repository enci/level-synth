#pragma once

#include <memory>
#include <random>
#include <string>
#include <unordered_map>

#include "pin.hpp"

namespace ls {

class layered_grid;

class eval_context {
public:
    double input_number(const std::string& pin_name) const;
    const layered_grid& input_grid(const std::string& pin_name) const;
    bool has_input(const std::string& pin_name) const;
    const pin_value& input_raw(const std::string& pin_name) const;
    void set_output_number(const std::string& pin_name, double value);
    void set_output_grid(const std::string& pin_name, std::shared_ptr<layered_grid> grid);
    std::mt19937& rng() { return m_rng; }

private:
    friend class eval_engine;

    std::unordered_map<std::string, pin_value> m_inputs;
    std::unordered_map<std::string, pin_value> m_outputs;
    std::mt19937 m_rng;
};

}