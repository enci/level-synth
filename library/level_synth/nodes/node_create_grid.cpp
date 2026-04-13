//
// Created by Bojan Endrovski on 13/04/2026.
//

#include "node_create_grid.hpp"
#include "../eval_context.hpp"
#include "../attribute_grid.hpp"

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

eval_task node_create_grid::evaluate(eval_context& ctx) { co_return; }

#ifdef LS_EDITOR
void node_create_grid::draw_ui() {}
#endif

} // namespace ls
