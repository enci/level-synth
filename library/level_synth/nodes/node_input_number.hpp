#pragma once

#include "../node.hpp"

namespace ls {

class node_input_number : public node {
public:
    const node_descriptor& descriptor() const override;
    bool evaluate(eval_context& ctx) override;
    void accept(node_visitor &v) override;

// protected:
    double m_value;
};

}
