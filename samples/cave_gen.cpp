// Cave generator sample
//
// Builds a small node graph that generates a random noise grid, refines it
// with cellular automata, and prints the result as ASCII art.
//
// Graph:
//   [InputNumber "iterations"] ------> [CellularAutomata] ----> [OutputGrid "level"]
//              [NoiseGrid] ----------^

#include <level_synth/level_synth.hpp>

#include <iostream>

int main() {
    ls::generator gen;
    ls::eval_engine& engine = gen.engine();


    // Input: number of CA iterations
    auto iterations_node = std::make_unique<ls::node_input_number>();
    iterations_node->param_name = "iterations";
    iterations_node->default_value = 4;
    int iterations_id = engine.add_node(std::move(iterations_node));

    // Random noise grid — ~45% density, 48x24
    auto noise_node = std::make_unique<ls::node_noise_grid>();
    noise_node->default_width = 48;
    noise_node->default_height = 24;
    noise_node->default_density = 0.45;
    int noise_id = engine.add_node(std::move(noise_node));

    // Cellular automata: sculpts caves out of the noise
    auto ca_node = std::make_unique<ls::node_cellular_automata>();
    ca_node->default_birth = 5;
    ca_node->default_death = 4;
    int ca_id = engine.add_node(std::move(ca_node));

    // Output: named "level" so we can retrieve it from the generator
    auto out_node = std::make_unique<ls::node_output_grid>();
    out_node->output_name = "level";
    int out_id = engine.add_node(std::move(out_node));

    // -- Wire them up --
    engine.add_wire({noise_id, "grid", ca_id, "input"});
    engine.add_wire({iterations_id, "value", ca_id, "iterations"});
    engine.add_wire({ca_id, "output", out_id, "value"});

    // Let the generator discover the input/output nodes
    gen.rebuild_bindings();

    // -- Run --
    gen.set_seed(42);
    gen.set_parameter("iterations", 6);
    gen.evaluate();

    // -- Print the result --
    auto grid = gen.get_grid_output("level");
    if (!grid) {
        std::cerr << "No output grid produced.\n";
        return 1;
    }

    for (int y = 0; y < grid->height(); y++) {
        for (int x = 0; x < grid->width(); x++) {
            int v = grid->get("terrain", x, y);
            std::cout << (v ? '#' : '.');
        }
        std::cout << '\n';
    }

    return 0;
}
