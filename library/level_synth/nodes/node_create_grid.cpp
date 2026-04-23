#include "node_create_grid.hpp"
#include "../eval_context.hpp"
#include "../grid.hpp"
#include "../node_registry.hpp"
#include "../node_visitor.hpp"
#ifdef LS_EDITOR
#include <imgui.h>
#include <cstring>
#endif

namespace ls {

const node_descriptor& node_create_grid::descriptor() const {
    static node_descriptor desc{
        "Create Grid", "Generation",
        {
            {"width",      pin_direction::input,  pin_type::number, true},
            {"height",     pin_direction::input,  pin_type::number, true},
            {"fill_value", pin_direction::input,  pin_type::number, false},
            {"grid",       pin_direction::output, pin_type::grid,   true},
        }
    };
    return desc;
}

bool node_create_grid::evaluate(eval_context& ctx) {
    if (ctx.has_input("width"))      m_width      = ctx.input_number("width");
    if (ctx.has_input("height"))     m_height     = ctx.input_number("height");
    if (ctx.has_input("fill_value")) m_fill_value = static_cast<int>(ctx.input_number("fill_value"));

    auto gr = std::make_shared<grid>(static_cast<int>(m_width), static_cast<int>(m_height), m_fill_value);
    ctx.set_output_grid("grid", std::move(gr));
    return true;
}

void node_create_grid::accept(node_visitor &v) {
    node::accept(v);
    v.visit("Width", m_width);
    v.visit("Height", m_height);
    v.visit("Fill Value", m_fill_value);
}

} // namespace ls
