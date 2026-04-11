#pragma once

#include <string>
#include <vector>

#include "pin.hpp"
#include "eval_task.hpp"

namespace ls {

class eval_context;

struct node_descriptor {
    std::string name;
    std::string category;
    std::vector<pin_descriptor> pins;
};

class node {
public:
    virtual ~node() = default;

    virtual const node_descriptor& descriptor() const = 0;
    virtual eval_task evaluate(eval_context& ctx) = 0;

#ifdef LS_EDITOR
    virtual void draw_ui() {}
#endif

    int id() const { return m_id; }

private:
    friend class eval_engine;
    int m_id = 0;
};

}