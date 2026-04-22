#include "node_input_number.hpp"
#include "../eval_context.hpp"
#include "../node_registry.hpp"
#ifdef LS_EDITOR
#include <imgui.h>
#include <cstring>
#endif

namespace ls {

const node_descriptor& node_input_number::descriptor() const {
    static node_descriptor desc = {
        .name = "Input Number",
        .category = "IO",
        .pins = {
            { "value", pin_direction::output, pin_type::number, true },
        }
    };
    return desc;
}

bool node_input_number::evaluate(eval_context& ctx) {
}

} // namespace ls
