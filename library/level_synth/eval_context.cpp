//
// Created by Bojan Endrovski on 13/04/2026.
//

#include "eval_context.hpp"
#include "attribute_grid.hpp"

namespace ls {

int eval_context::input_int(const std::string& pin_name) const { return 0; }

float eval_context::input_float(const std::string& pin_name) const { return 0; }

const attribute_grid& eval_context::input_grid(const std::string& pin_name) const {
    throw std::runtime_error("not implemented");
}

bool eval_context::has_input(const std::string& pin_name) const { return false; }

void eval_context::set_output_int(const std::string& pin_name, int value) {}

void eval_context::set_output_float(const std::string& pin_name, float value) {}

void eval_context::set_output_grid(const std::string& pin_name, std::shared_ptr<attribute_grid> grid) {}

} // namespace ls