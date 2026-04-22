#pragma once

#include "../node.hpp"

namespace ls {

class node_input_number : public node {
public:
    const node_descriptor& descriptor() const override;
    bool evaluate(eval_context& ctx) override;

#ifdef LS_EDITOR
    void edit() override;
#endif
};

}
