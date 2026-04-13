#pragma once

#include "../node.hpp"

namespace ls {

class node_generator_input : public node {
public:
    const node_descriptor& descriptor() const override;
    eval_task evaluate(eval_context& ctx) override;

#ifdef LS_EDITOR
    void draw_ui() override;
#endif

    std::string param_name = "param";
    double default_value = 0;

    // Runtime override — set by generator::set_parameter()
    bool has_override = false;
    double override_value = 0;
};

}