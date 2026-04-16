#pragma once

#include "../node.hpp"

namespace ls {

class node_create_grid : public node {
public:
    const node_descriptor& descriptor() const override;
    eval_task evaluate(eval_context& ctx) override;

#ifdef LS_EDITOR
    void draw_ui() override;
    bool has_input_default(const std::string& pin_name) const override;
    double get_default(const std::string& pin_name) const override;
    void set_default(const std::string& pin_name, double value) override;
#endif

    double default_width = 64;
    double default_height = 64;
    double default_fill = 0;
    std::string attribute_name = "terrain";
};

} // namespace ls
