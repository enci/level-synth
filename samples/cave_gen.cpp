// Cave generator sample
//
// Builds a small node graph that generates a random noise grid, refines it
// with cellular automata, and prints the result as ASCII art.
// Uses the generator API to expose named parameters and outputs.
//
// Graph:
//   [InputNumber "seed_density"] --> [NoiseGrid]
//            [CreateGrid] --------^      |
//                                   [CellularAutomata] --> [OutputGrid "level"]

#include <level_synth/level_synth.hpp>
#include <level_synth/node_graph.hpp>

#include <iostream>

int main() {
    ls::generator gen;
    ls::node_graph& graph = gen.graph();

    // Blank 64x32 grid
    auto create = std::make_unique<ls::node_create_grid>();
    create->m_width  = 64;
    create->m_height = 32;
    int create_id = graph.add_node(std::move(create));

    // Expose noise density as a named parameter
    auto density_in = std::make_unique<ls::node_input_number>();
    density_in->m_value = 0.45;
    density_in->set_name("density");
    int density_id = graph.add_node(std::move(density_in));

    // Random noise
    auto noise = std::make_unique<ls::node_noise_grid>();
    noise->density = 0.45;
    int noise_id = graph.add_node(std::move(noise));

    // Cellular automata
    auto ca = std::make_unique<ls::node_cellular_automata>();
    ca->m_iterations = 5;
    ca->m_birth      = 5;
    ca->m_death      = 4;
    int ca_id = graph.add_node(std::move(ca));

    // Named output
    auto out = std::make_unique<ls::node_output_grid>();
    out->set_name("level");
    int out_id = graph.add_node(std::move(out));

    graph.add_wire({create_id,  "grid",   noise_id, "grid"  });
    graph.add_wire({noise_id,   "grid",   ca_id,    "input" });
    graph.add_wire({ca_id,      "output", out_id,   "value" });

    gen.rebuild_bindings();

    gen.set_seed(42);
    gen.set_parameter("density", 0.45); // override default
    gen.evaluate();

    auto grid = gen.get_grid_output("level");
    if (!grid) {
        std::cerr << "No output produced.\n";
        return 1;
    }

    for (int y = 0; y < grid->height(); ++y) {
        for (int x = 0; x < grid->width(); ++x)
            std::cout << (grid->get(x, y) ? '#' : '.');
        std::cout << '\n';
    }

    return 0;
}
