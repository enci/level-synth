#include "eval_context.hpp"

#include <stdexcept>

namespace ls {

double eval_context::input_number(const std::string& pin_name) const {
    auto it = m_inputs.find(pin_name);
    if (it == m_inputs.end()) throw std::runtime_error("Missing input: " + pin_name);
    return std::get<double>(it->second);
}

const grid& eval_context::input_grid(const std::string& pin_name) const {
    auto it = m_inputs.find(pin_name);
    if (it == m_inputs.end()) throw std::runtime_error("Missing input: " + pin_name);
    return *std::get<std::shared_ptr<grid>>(it->second);
}

const pin_value& eval_context::input_raw(const std::string& pin_name) const {
    auto it = m_inputs.find(pin_name);
    if (it == m_inputs.end()) throw std::runtime_error("Missing input: " + pin_name);
    return it->second;
}

bool eval_context::has_input(const std::string& pin_name) const {
    return m_inputs.contains(pin_name);
}

void eval_context::set_output_number(const std::string& pin_name, double value) {
    m_outputs[pin_name] = value;
}

void eval_context::set_output_grid(const std::string& pin_name, std::shared_ptr<grid> grid) {
    m_outputs[pin_name] = std::move(grid);
}

}
