#include "node_create_grid.hpp"
#include "../eval_context.hpp"
#include "../layered_grid.hpp"
#include "../node_registry.hpp"
#include "level_synth/layered_grid.hpp"
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
    // auto w = ctx.has_input("width") ? ctx.input_number("width") : m_width;

    auto l_grid = std::make_shared<layered_grid>(m_width, m_height);
    auto& grid =(*l_grid)[m_layer_name];
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            grid(x, y) = m_fill_value;
        }
    }

    ctx.set_output_grid("grid", std::move(l_grid));
}

#ifdef LS_EDITOR
void node_create_grid::edit() {
    /*
    int w    = static_cast<int>(default_width);
    int h    = static_cast<int>(default_height);
    int fill = static_cast<int>(default_fill);
    if (ImGui::DragInt("Width",  &w,    1, 1, 512)) default_width  = w;
    if (ImGui::DragInt("Height", &h,    1, 1, 512)) default_height = h;
    if (ImGui::DragInt("Fill",   &fill, 1, 0, 255)) default_fill   = fill;

    char buf[128];
    std::strncpy(buf, attribute_name.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Attribute", buf, sizeof(buf)))
        attribute_name = buf;
    */
}

#endif

} // namespace ls
