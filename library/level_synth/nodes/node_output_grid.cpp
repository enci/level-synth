#include "node_output_grid.hpp"
#include "../eval_context.hpp"
#include "../node_registry.hpp"

namespace ls {

const node_descriptor& node_output_grid::descriptor() const {
    static node_descriptor desc = {
        .pins = {
            { "value", pin_direction::input, pin_type::grid, true },
        }
    };
    return desc;
}

bool node_output_grid::evaluate(eval_context& ctx) {
    if (!ctx.has_input("value")) return false;
    ctx.set_output_grid("value",
        std::get<std::shared_ptr<grid>>(ctx.input_raw("value")));
    return true;
}

LS_REGISTER_NODE(node_output_grid, "Output Grid", "IO");

}
