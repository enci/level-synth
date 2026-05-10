#pragma once

#include "../node.hpp"
#include "../tag.hpp"

namespace ls {

class node_create_grid : public node {
public:
    const node_descriptor& descriptor() const override;
    bool evaluate(eval_context& ctx) override;
    void accept(node_visitor &v) override;

protected:
    double m_width = 64;
    double m_height = 64;
    tag m_fill_value = {};
};

}
