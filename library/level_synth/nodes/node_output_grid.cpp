#include "node_output_grid.hpp"
#include "../eval_context.hpp"
#include "../node_registry.hpp"
#ifdef LS_EDITOR
#include <imgui.h>
#include <cstring>
#endif

namespace ls {

const node_descriptor& node_output_grid::descriptor() const {
    static node_descriptor desc = {
        .name = "Output Grid",
        .category = "IO",
        .pins = {
            { "value", pin_direction::input, pin_type::grid, true },
        }
    };
    return desc;
}

eval_task node_output_grid::evaluate(eval_context& ctx) {
    if (!ctx.has_input("value")) co_return;
    ctx.set_output_grid("value",
        std::get<std::shared_ptr<layered_grid>>(ctx.input_raw("value")));
    co_return;
}

#ifdef LS_EDITOR
void node_output_grid::draw_ui() {
    char buf[128];
    std::strncpy(buf, output_name.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Name", buf, sizeof(buf)))
        output_name = buf;
}
#endif

LS_REGISTER_NODE(node_output_grid);

} // namespace ls
