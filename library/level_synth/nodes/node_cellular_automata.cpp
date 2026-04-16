#include "node_cellular_automata.hpp"
#include "../eval_context.hpp"
#include "../attribute_grid.hpp"
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
    int iter  = static_cast<int>(default_iterations);
    int birth = static_cast<int>(default_birth);
    int death = static_cast<int>(default_death);
    if (ImGui::DragInt("Iterations", &iter,  1, 1, 100)) default_iterations = iter;
    if (ImGui::DragInt("Birth",      &birth, 1, 0, 8))   default_birth      = birth;
    if (ImGui::DragInt("Death",      &death, 1, 0, 8))   default_death      = death;

    char buf[128];
    std::strncpy(buf, attribute_name.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Attribute", buf, sizeof(buf)))
        attribute_name = buf;
}

bool node_cellular_automata::has_input_default(const std::string& pin_name) const {
    return pin_name == "iterations" || pin_name == "birth" || pin_name == "death";
}

double node_cellular_automata::get_default(const std::string& pin_name) const {
    if (pin_name == "iterations") return default_iterations;
    if (pin_name == "birth")      return default_birth;
    if (pin_name == "death")      return default_death;
    return 0.0;
}

void node_cellular_automata::set_default(const std::string& pin_name, double value) {
    if      (pin_name == "iterations") default_iterations = value;
    else if (pin_name == "birth")      default_birth      = value;
    else if (pin_name == "death")      default_death      = value;
}
#endif

LS_REGISTER_NODE(node_cellular_automata);

}