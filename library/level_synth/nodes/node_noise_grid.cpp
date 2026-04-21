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
            {"grid",    pin_direction::input,  pin_type::grid,   true},
            {"density", pin_direction::input,  pin_type::number, false},
            {"grid",    pin_direction::output, pin_type::grid,   true},
        }
    };
    return desc;
}

eval_task node_noise_grid::evaluate(eval_context& ctx) {
    if (!ctx.has_input("grid")) co_return;

    double density = ctx.has_input("density")
        ? ctx.input_number("density")
        : default_density;

    const auto& src = ctx.input_grid("grid");
    auto grid = std::make_shared<attribute_grid>(src);
    grid->add_attribute(attribute_name, 0);

    int w = grid->width();
    int h = grid->height();

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
    float density = static_cast<float>(default_density);
    if (ImGui::SliderFloat("Density", &density, 0.0f, 1.0f))   default_density = density;

    char buf[128];
    std::strncpy(buf, attribute_name.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Attribute", buf, sizeof(buf)))
        attribute_name = buf;
}

bool node_noise_grid::has_input_default(const std::string& pin_name) const {
    return pin_name == "density";
}

double node_noise_grid::get_default(const std::string& pin_name) const {
    if (pin_name == "density") return default_density;
    return 0.0;
}

void node_noise_grid::set_default(const std::string& pin_name, double value) {
    if (pin_name == "density") default_density = value;
}
#endif

LS_REGISTER_NODE(node_noise_grid);

} // namespace ls
