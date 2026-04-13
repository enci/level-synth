//
// Created by Bojan Endrovski on 13/04/2026.
//

#include "node_generator_input.hpp"
#include "../eval_context.hpp"

namespace ls {

const node_descriptor& node_generator_input::descriptor() const {
    static node_descriptor desc{
        "Generator Input", "IO",
        {
            {"value", pin_direction::output, pin_type::number, true},
        }
    };
    return desc;
}

eval_task node_generator_input::evaluate(eval_context& ctx) { co_return; }

#ifdef LS_EDITOR
void node_generator_input::draw_ui() {}
#endif

} // namespace ls
