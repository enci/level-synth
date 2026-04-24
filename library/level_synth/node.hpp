#pragma once

#include <string>
#include <vector>

#include "pin.hpp"
#include "vec2.hpp"

namespace ls {

class eval_context;
class node_graph;
class node_visitor;

struct node_descriptor { std::vector<pin_descriptor> pins; };

class node {
public:
    virtual ~node() = default;
    virtual const node_descriptor& descriptor() const = 0;
    virtual bool evaluate(eval_context& ctx) = 0;
    int id() const { return m_id; }
    virtual void accept(node_visitor& v);

    const std::string& name() const { return m_name; }
    void set_name(std::string name) { m_name = std::move(name); }

    vec2 position() const { return m_position; }
    void set_position(vec2 p) { m_position = p; }

protected:
    friend class node_graph;
    int m_id = 0;
    vec2 m_position;
    std::string m_name;
};

}