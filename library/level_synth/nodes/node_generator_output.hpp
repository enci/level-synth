#pragma once

#include "../node.hpp"

namespace ls {

class node_generator_output : public node {
public:
    const node_descriptor& descriptor() const override;
    eval_task evaluate(eval_context& ctx) override;

#ifdef LS_EDITOR
    void draw_ui() override;
#endif

    std::string output_name = "output";
    pin_type output_type = pin_type::grid;

    // Result stored after evaluation
    pin_value result;
};

}