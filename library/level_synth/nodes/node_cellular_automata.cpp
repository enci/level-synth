#include "node_cellular_automata.hpp"
#include "../eval_context.hpp"
#include "../attribute_grid.hpp"

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

static int count_neighbors(const attribute_grid& grid, const std::string& attr, int x, int y) {
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx;
            int ny = y + dy;
            if (grid.in_bounds(nx, ny)) {
                if (grid.get(attr, nx, ny) != 0) count++;
            } else {
                // Out-of-bounds counts as alive (wall border)
                count++;
            }
        }
    }
    return count;
}

eval_task node_cellular_automata::evaluate(eval_context& ctx) {
    const auto& input = ctx.input_grid("input");

    int iterations = ctx.has_input("iterations")
        ? static_cast<int>(ctx.input_number("iterations"))
        : static_cast<int>(default_iterations);

    int birth = ctx.has_input("birth")
        ? static_cast<int>(ctx.input_number("birth"))
        : static_cast<int>(default_birth);

    int death = ctx.has_input("death")
        ? static_cast<int>(ctx.input_number("death"))
        : static_cast<int>(default_death);

    auto output = std::make_shared<attribute_grid>(input);

    for (int i = 0; i < iterations; i++) {
        // Snapshot current state for neighbor reads
        attribute_grid prev(*output);

        int w = output->width();
        int h = output->height();

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int neighbors = count_neighbors(prev, attribute_name, x, y);
                int alive = prev.get(attribute_name, x, y) != 0;

                if (!alive && neighbors >= birth)
                    output->set(attribute_name, x, y, 1);
                else if (alive && neighbors < death)
                    output->set(attribute_name, x, y, 0);
            }
        }

        co_yield eval_step{
            .label = "Iteration " + std::to_string(i + 1) + "/" + std::to_string(iterations),
            .progress = static_cast<float>(i + 1) / static_cast<float>(iterations)
        };
    }

    ctx.set_output_grid("output", std::move(output));
    co_return;
}

#ifdef LS_EDITOR
void node_cellular_automata::draw_ui() {
    // TODO: sliders for default_iterations, default_birth, default_death
    // TODO: text input for attribute_name
}
#endif

}