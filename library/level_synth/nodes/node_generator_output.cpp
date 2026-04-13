//
// Created by Bojan Endrovski on 13/04/2026.
//

#include "node_generator_output.hpp"
#include "../eval_context.hpp"

namespace ls {

const node_descriptor& node_generator_output::descriptor() const {
    static node_descriptor desc{
        "Generator Output", "IO",
        {
            {"value", pin_direction::input, pin_type::grid, true},
        }
    };
    return desc;
}

eval_task node_generator_output::evaluate(eval_context& ctx) { co_return; }

#ifdef LS_EDITOR
void node_generator_output::draw_ui() {}
#endif

} // namespace ls
