#include "node_create_grid.hpp"
#include "../eval_context.hpp"
#include "../attribute_grid.hpp"
#include "../node_registry.hpp"

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

eval_task node_create_grid::evaluate(eval_context& ctx) {
    int w = ctx.has_input("width")
        ? static_cast<int>(ctx.input_number("width"))
        : static_cast<int>(default_width);

    int h = ctx.has_input("height")
        ? static_cast<int>(ctx.input_number("height"))
        : static_cast<int>(default_height);

    int fill = ctx.has_input("fill_value")
        ? static_cast<int>(ctx.input_number("fill_value"))
        : static_cast<int>(default_fill);

    auto grid = std::make_shared<attribute_grid>(w, h);
    grid->add_attribute(attribute_name, fill);

    ctx.set_output_grid("grid", std::move(grid));
    co_return;
}

#ifdef LS_EDITOR
void node_create_grid::draw_ui() {}
#endif

LS_REGISTER_NODE(node_create_grid);

} // namespace ls
