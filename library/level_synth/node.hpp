#pragma once

#include <string>
#include <vector>

#include "pin.hpp"

namespace ls {

class eval_context;
class node_graph;

struct node_descriptor {
    std::string name;
    std::string category;
    std::vector<pin_descriptor> pins;
};

class node {
public:
    virtual ~node() = default;
    virtual const node_descriptor& descriptor() const = 0;
    virtual bool evaluate(eval_context& ctx) = 0;
    int id() const { return m_id; }
    const std::string& name() const { return m_name; }

#ifdef LS_EDITOR
    virtual void edit() {}
#endif

private:
    //friend class eval_engine;
    friend class node_graph;
    int m_id = 0;
    std::string m_name;
};

}