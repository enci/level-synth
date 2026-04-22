#pragma once

#include "../node.hpp"

namespace ls {

class node_create_grid : public node {
public:
    const node_descriptor& descriptor() const override;
    bool evaluate(eval_context& ctx) override;

// protected:
    double m_width = 64;
    double m_height = 64;
    int m_fill_value = 0;
    std::string m_layer_name = "values";
};

} // namespace ls
