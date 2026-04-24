#include "node.hpp"
#include "node_visitor.hpp"

namespace ls {

void node::accept(node_visitor& v) {
    v.visit("name",     m_name);
    v.visit("position", m_position);
}

}
