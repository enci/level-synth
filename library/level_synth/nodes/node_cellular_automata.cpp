#include "node_cellular_automata.hpp"
#include "../eval_context.hpp"
#include "../grid.hpp"
#include "../node_registry.hpp"
#ifdef LS_EDITOR
#include <imgui.h>
#include <cstring>
#endif

namespace ls {

const node_descriptor& node_cellular_automata::descriptor() const {
    static node_descriptor desc = {
        .name = "Cellular Automata",
        .category = "Generation",
        .pins = {
            { "input",      pin_direction::input,  pin_type::grid,   true  },
            { "iterations", pin_direction::input,  pin_type::number, false },
            { "birth",      pin_direction::input,  pin_type::number, false },
            { "death",      pin_direction::input,  pin_type::number, false },
            { "output",     pin_direction::output, pin_type::grid,   true  },
        }
    };
    return desc;
}

static int count_neighbors(const grid& grid, int x, int y) {
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx;
            int ny = y + dy;
            if (grid.in_bounds(nx, ny)) {
                if (grid.get(nx, ny) != 0) count++;
            } else {
                // Out-of-bounds counts as alive (wall border)
                count++;
            }
        }
    }
    return count;
}

bool node_cellular_automata::evaluate(eval_context& ctx) {
    if (ctx.has_input("iterations")) m_iterations = ctx.input_number("iterations");
    if (ctx.has_input("birth"))      m_birth      = ctx.input_number("birth");
    if (ctx.has_input("death"))      m_death      = ctx.input_number("death");

    const auto& input = ctx.input_grid("input");
    auto output = std::make_shared<grid>(input);

    for (int i = 0; i < static_cast<int>(m_iterations); i++) {

        // Snapshot current state for neighbor reads
        grid prev(*output);

        int w = output->width();
        int h = output->height();

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int neighbors = count_neighbors(prev, x, y);
                int alive = prev.get(x, y) != 0;

                if (!alive && neighbors >= m_birth)
                    output->set(x, y, 1);
                else if (alive && neighbors < m_death)
                    output->set(x, y, 0);
            }
        }
    }

    ctx.set_output_grid("output", std::move(output));
    return true;
}

}