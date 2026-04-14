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

eval_task node_output_number::evaluate(eval_context& ctx) {
    if (!ctx.has_input("value")) co_return;
    ctx.set_output_number("value", ctx.input_number("value"));
    co_return;
}

#ifdef LS_EDITOR
void node_output_number::draw_ui() {
    char buf[128];
    std::strncpy(buf, output_name.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Name", buf, sizeof(buf)))
        output_name = buf;
}
#endif

LS_REGISTER_NODE(node_output_number);

} // namespace ls
