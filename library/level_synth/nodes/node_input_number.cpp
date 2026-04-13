#include "node_input_number.hpp"
#include "../eval_context.hpp"
#include "../node_registry.hpp"

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

eval_task node_input_number::evaluate(eval_context& ctx) {
    double value = has_override ? override_value : default_value;
    ctx.set_output_number("value", value);
    co_return;
}

#ifdef LS_EDITOR
void node_input_number::draw_ui() {
}
#endif

LS_REGISTER_NODE(node_input_number);

} // namespace ls
