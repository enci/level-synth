#include "node_output_number.hpp"
#include "../eval_context.hpp"
#include "../node_registry.hpp"
#ifdef LS_EDITOR
#include <imgui.h>
#include <cstring>
#endif

namespace ls {

const node_descriptor& node_output_number::descriptor() const {
    static node_descriptor desc = {
        .name = "Output Number",
        .category = "IO",
        .pins = {
            { "value", pin_direction::input, pin_type::number, true },
        }
    };
    return desc;
}

bool node_output_number::evaluate(eval_context& ctx) {
    ctx.set_output_number("value", ctx.input_number("value"));
    return true;
}

} // namespace ls
