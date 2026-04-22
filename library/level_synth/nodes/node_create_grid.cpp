#include "node_create_grid.hpp"
#include "../eval_context.hpp"
#include "../grid.hpp"
#include "../node_registry.hpp"
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
    auto gr = std::make_shared<grid>(m_width, m_height, m_fill_value);
    ctx.set_output_grid("grid", std::move(gr));
    return true;
}

} // namespace ls
