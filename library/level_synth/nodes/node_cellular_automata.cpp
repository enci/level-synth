//
// Created by Bojan Endrovski on 13/04/2026.
//

#include "node_cellular_automata.hpp"
#include "../eval_context.hpp"
#include "../attribute_grid.hpp"

namespace ls {

const node_descriptor& node_cellular_automata::descriptor() const {
    static node_descriptor desc{
        "Cellular Automata", "Generation",
        {
            {"input",      pin_direction::input,  pin_type::grid,   true},
            {"iterations", pin_direction::input,  pin_type::number, false},
            {"birth",      pin_direction::input,  pin_type::number, false},
            {"death",      pin_direction::input,  pin_type::number, false},
            {"output",     pin_direction::output, pin_type::grid,   true},
        }
    };
    return desc;
}

eval_task node_cellular_automata::evaluate(eval_context& ctx) { co_return; }

#ifdef LS_EDITOR
void node_cellular_automata::draw_ui() {}
#endif

} // namespace ls
