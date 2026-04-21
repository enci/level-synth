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
    virtual bool has_input_default(const std::string& /*pin_name*/) const { return false; }
    virtual double get_default(const std::string& /*pin_name*/) const { return 0.0; }
    virtual void set_default(const std::string& /*pin_name*/, double /*value*/) {}
    virtual bool draw_body_ui() { return false; }
#endif

    int id() const { return m_id; }

private:
    friend class eval_engine;
    int m_id = 0;
};

}