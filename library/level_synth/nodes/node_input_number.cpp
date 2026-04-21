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

eval_task node_input_number::evaluate(eval_context& ctx) {
    double value = has_override ? override_value : default_value;
    ctx.set_output_number("value", value);
    co_return;
}

#ifdef LS_EDITOR
void node_input_number::draw_ui() {
    char buf[128];
    std::strncpy(buf, param_name.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Name", buf, sizeof(buf)))
        param_name = buf;

    float val = static_cast<float>(default_value);
    if (ImGui::DragFloat("Default", &val, 0.1f))
        default_value = val;
}

bool node_input_number::draw_body_ui() {
    float val = static_cast<float>(default_value);
    ImGui::SetNextItemWidth(80.0f);
    bool changed = ImGui::DragFloat("##val", &val, 0.1f);
    if (changed) default_value = static_cast<double>(val);
    return changed;
}
#endif

LS_REGISTER_NODE(node_input_number);

} // namespace ls
