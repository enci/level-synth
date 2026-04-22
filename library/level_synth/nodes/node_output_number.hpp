#pragma once

#include "../node.hpp"

namespace ls {

class node_output_number : public node {
public:
    const node_descriptor& descriptor() const override;
    bool evaluate(eval_context& ctx) override;

    /*
#ifdef LS_EDITOR
    void draw_ui() override;
#endif

    std::string output_name = "output";
    */
};

}
