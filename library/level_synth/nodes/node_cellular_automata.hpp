#pragma once

#include "../node.hpp"

namespace ls {

class node_cellular_automata : public node {
public:
    const node_descriptor& descriptor() const override;
    bool evaluate(eval_context& ctx) override;
    void accept(node_visitor &v) override;

    double m_iterations = 5;
    double m_birth = 5;
    double m_death = 3;
};

}