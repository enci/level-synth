#include "node_noise_grid.hpp"
#include "../eval_context.hpp"
#include "../attribute_grid.hpp"
#include "../node_registry.hpp"
#ifdef LS_EDITOR
#include <imgui.h>
#include <cstring>
#endif
#include <random>

namespace ls {

const node_descriptor& node_noise_grid::descriptor() const {
    static node_descriptor desc{
        "Noise Grid", "Generation",
        {
            {"width",   pin_direction::input,  pin_type::number, false},
            {"height",  pin_direction::input,  pin_type::number, false},
            {"density", pin_direction::input,  pin_type::number, false},
            {"grid",    pin_direction::output, pin_type::grid,   true},
        }
    };
    return desc;
}

eval_task node_noise_grid::evaluate(eval_context& ctx) {
    int w = ctx.has_input("width")
        ? static_cast<int>(ctx.input_number("width"))
        : static_cast<int>(default_width);

    int h = ctx.has_input("height")
        ? static_cast<int>(ctx.input_number("height"))
        : static_cast<int>(default_height);

    double density = ctx.has_input("density")
        ? ctx.input_number("density")
        : default_density;

    auto grid = std::make_shared<attribute_grid>(w, h);
    grid->add_attribute(attribute_name, 0);

    std::uniform_real_distribution<double> dist(0.0, 1.0);
    auto& rng = ctx.rng();

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (dist(rng) < density)
                grid->set(attribute_name, x, y, 1);
        }
    }

    ctx.set_output_grid("grid", std::move(grid));
    co_return;
}

#ifdef LS_EDITOR
void node_noise_grid::draw_ui() {
    int w = static_cast<int>(default_width);
    int h = static_cast<int>(default_height);
    float density = static_cast<float>(default_density);
    if (ImGui::DragInt("Width",   &w, 1, 1, 512))              default_width   = w;
    if (ImGui::DragInt("Height",  &h, 1, 1, 512))              default_height  = h;
    if (ImGui::SliderFloat("Density", &density, 0.0f, 1.0f))   default_density = density;

    char buf[128];
    std::strncpy(buf, attribute_name.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Attribute", buf, sizeof(buf)))
        attribute_name = buf;
}
#endif

LS_REGISTER_NODE(node_noise_grid);

} // namespace ls
