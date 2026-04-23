#include "node_noise_grid.hpp"
#include "../eval_context.hpp"
#include "../grid.hpp"
#include "../node_registry.hpp"
#ifdef LS_EDITOR
#include <imgui.h>
#include <cstring>
#endif
#include <random>

#include "level_synth/node_visitor.hpp"

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

bool node_noise_grid::evaluate(eval_context& ctx) {
    if (!ctx.has_input("grid")) return false;
    if (ctx.has_input("density")) m_density = ctx.input_number("density");

    const grid& src = ctx.input_grid("grid");
    auto gr = std::make_shared<grid>(src);

    int w = gr->width();
    int h = gr->height();

    std::uniform_real_distribution<double> dist(0.0, 1.0);
    auto& rng = ctx.rng();

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (dist(rng) < m_density)
                gr->set(x, y, 1);
        }
    }

    ctx.set_output_grid("grid", std::move(gr));
    return true;
}

void node_noise_grid::accept(node_visitor& v) {
    node::accept(v);
    v.visit("Density", m_density);
}

} // namespace ls
