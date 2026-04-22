#include <catch2/catch_test_macros.hpp>
#include <level_synth/level_synth.hpp>
#include <level_synth/node_graph.hpp>

// ---- grid ---------------------------------------------------------------

TEST_CASE("grid default construction", "[grid]") {
    ls::grid g(10, 8);
    CHECK(g.width() == 10);
    CHECK(g.height() == 8);
}

TEST_CASE("grid fill construction", "[grid]") {
    ls::grid g(4, 4, 7);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            CHECK(g.get(x, y) == 7);
}

TEST_CASE("grid get and set", "[grid]") {
    ls::grid g(5, 5);
    g.set(2, 3, 42);
    CHECK(g.get(2, 3) == 42);
    CHECK(g.get(0, 0) == 0);
}

TEST_CASE("grid operator()", "[grid]") {
    ls::grid g(5, 5);
    g(1, 2) = 99;
    CHECK(g(1, 2) == 99);
    CHECK(static_cast<const ls::grid&>(g)(1, 2) == 99);
}

TEST_CASE("grid in_bounds", "[grid]") {
    ls::grid g(5, 5);
    CHECK(g.in_bounds(0, 0));
    CHECK(g.in_bounds(4, 4));
    CHECK_FALSE(g.in_bounds(-1, 0));
    CHECK_FALSE(g.in_bounds(0, -1));
    CHECK_FALSE(g.in_bounds(5, 0));
    CHECK_FALSE(g.in_bounds(0, 5));
}

TEST_CASE("grid fill clears all cells", "[grid]") {
    ls::grid g(3, 3);
    g.set(1, 1, 5);
    g.fill(0);
    CHECK(g.get(1, 1) == 0);
}

TEST_CASE("grid copy is independent", "[grid]") {
    ls::grid a(3, 3, 1);
    a.set(1, 1, 9);
    ls::grid b(a);
    CHECK(b.get(1, 1) == 9);
    b.set(1, 1, 0);
    CHECK(a.get(1, 1) == 9);
}

// ---- node_graph ---------------------------------------------------------

TEST_CASE("node_graph add and find node", "[graph]") {
    ls::node_graph graph;
    int id = graph.add_node(std::make_unique<ls::node_create_grid>());
    CHECK(graph.find_node(id) != nullptr);
    CHECK(graph.find_node(id + 1) == nullptr);
}

TEST_CASE("node_graph node IDs are unique and incrementing", "[graph]") {
    ls::node_graph graph;
    int a = graph.add_node(std::make_unique<ls::node_create_grid>());
    int b = graph.add_node(std::make_unique<ls::node_create_grid>());
    CHECK(a != b);
    CHECK(graph.node_ids().size() == 2);
}

TEST_CASE("node_graph remove node removes connected wires", "[graph]") {
    ls::node_graph graph;
    int a = graph.add_node(std::make_unique<ls::node_create_grid>());
    int b = graph.add_node(std::make_unique<ls::node_noise_grid>());
    graph.add_wire({a, "grid", b, "grid"});
    CHECK(graph.wires().size() == 1);
    graph.remove_node(a);
    CHECK(graph.wires().empty());
    CHECK(graph.find_node(a) == nullptr);
}

TEST_CASE("node_graph remove specific wire", "[graph]") {
    ls::node_graph graph;
    int a = graph.add_node(std::make_unique<ls::node_create_grid>());
    int b = graph.add_node(std::make_unique<ls::node_noise_grid>());
    graph.add_wire({a, "grid", b, "grid"});
    graph.remove_wire(a, "grid", b, "grid");
    CHECK(graph.wires().empty());
}

// ---- eval_engine --------------------------------------------------------

TEST_CASE("eval create_grid produces correct dimensions and fill", "[eval]") {
    ls::node_graph graph;
    auto n = std::make_unique<ls::node_create_grid>();
    n->m_width = 16;
    n->m_height = 12;
    n->m_fill_value = 3;
    int id = graph.add_node(std::move(n));

    ls::eval_engine engine;
    engine.evaluate(graph, 0);

    const auto* val = engine.get_output(id, "grid");
    REQUIRE(val != nullptr);
    auto g = std::get<std::shared_ptr<ls::grid>>(*val);
    REQUIRE(g != nullptr);
    CHECK(g->width() == 16);
    CHECK(g->height() == 12);
    CHECK(g->get(0, 0) == 3);
    CHECK(g->get(15, 11) == 3);
}

TEST_CASE("eval noise_grid output is all 0 or 1", "[eval]") {
    ls::node_graph graph;
    auto create = std::make_unique<ls::node_create_grid>();
    create->m_width = 20;
    create->m_height = 20;
    int cid = graph.add_node(std::move(create));

    auto noise = std::make_unique<ls::node_noise_grid>();
    noise->density = 0.5;
    int nid = graph.add_node(std::move(noise));

    graph.add_wire({cid, "grid", nid, "grid"});

    ls::eval_engine engine;
    engine.evaluate(graph, 42);

    const auto* val = engine.get_output(nid, "grid");
    REQUIRE(val != nullptr);
    auto g = std::get<std::shared_ptr<ls::grid>>(*val);
    REQUIRE(g != nullptr);
    CHECK(g->width() == 20);
    CHECK(g->height() == 20);
    for (int y = 0; y < g->height(); ++y)
        for (int x = 0; x < g->width(); ++x)
            CHECK((g->get(x, y) == 0 || g->get(x, y) == 1));
}

TEST_CASE("eval noise_grid density=0 produces all zeros", "[eval]") {
    ls::node_graph graph;
    auto create = std::make_unique<ls::node_create_grid>();
    create->m_width = 10;
    create->m_height = 10;
    int cid = graph.add_node(std::move(create));

    auto noise = std::make_unique<ls::node_noise_grid>();
    noise->density = 0.0;
    int nid = graph.add_node(std::move(noise));
    graph.add_wire({cid, "grid", nid, "grid"});

    ls::eval_engine engine;
    engine.evaluate(graph, 0);

    auto g = std::get<std::shared_ptr<ls::grid>>(*engine.get_output(nid, "grid"));
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x)
            CHECK(g->get(x, y) == 0);
}

TEST_CASE("eval noise_grid density=1 produces all ones", "[eval]") {
    ls::node_graph graph;
    auto create = std::make_unique<ls::node_create_grid>();
    create->m_width = 10;
    create->m_height = 10;
    int cid = graph.add_node(std::move(create));

    auto noise = std::make_unique<ls::node_noise_grid>();
    noise->density = 1.0;
    int nid = graph.add_node(std::move(noise));
    graph.add_wire({cid, "grid", nid, "grid"});

    ls::eval_engine engine;
    engine.evaluate(graph, 0);

    auto g = std::get<std::shared_ptr<ls::grid>>(*engine.get_output(nid, "grid"));
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x)
            CHECK(g->get(x, y) == 1);
}

TEST_CASE("eval cellular_automata preserves grid dimensions", "[eval]") {
    ls::node_graph graph;
    auto create = std::make_unique<ls::node_create_grid>();
    create->m_width = 32;
    create->m_height = 32;
    int cid = graph.add_node(std::move(create));

    auto noise = std::make_unique<ls::node_noise_grid>();
    noise->density = 0.45;
    int nid = graph.add_node(std::move(noise));

    auto ca = std::make_unique<ls::node_cellular_automata>();
    ca->m_iterations = 3;
    int caid = graph.add_node(std::move(ca));

    graph.add_wire({cid, "grid", nid, "grid"});
    graph.add_wire({nid, "grid", caid, "input"});

    ls::eval_engine engine;
    engine.evaluate(graph, 0);

    const auto* val = engine.get_output(caid, "output");
    REQUIRE(val != nullptr);
    auto g = std::get<std::shared_ptr<ls::grid>>(*val);
    REQUIRE(g != nullptr);
    CHECK(g->width() == 32);
    CHECK(g->height() == 32);
}

TEST_CASE("eval is deterministic with the same seed", "[eval]") {
    auto run = [](int seed) {
        ls::node_graph graph;
        auto create = std::make_unique<ls::node_create_grid>();
        create->m_width = 10;
        create->m_height = 10;
        int cid = graph.add_node(std::move(create));
        auto noise = std::make_unique<ls::node_noise_grid>();
        noise->density = 0.5;
        int nid = graph.add_node(std::move(noise));
        graph.add_wire({cid, "grid", nid, "grid"});
        ls::eval_engine engine;
        engine.evaluate(graph, seed);
        return std::get<std::shared_ptr<ls::grid>>(*engine.get_output(nid, "grid"));
    };

    auto g1 = run(123);
    auto g2 = run(123);
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x)
            CHECK(g1->get(x, y) == g2->get(x, y));
}

TEST_CASE("eval produces different results with different seeds", "[eval]") {
    auto run = [](int seed) {
        ls::node_graph graph;
        auto create = std::make_unique<ls::node_create_grid>();
        create->m_width = 20;
        create->m_height = 20;
        int cid = graph.add_node(std::move(create));
        auto noise = std::make_unique<ls::node_noise_grid>();
        noise->density = 0.5;
        int nid = graph.add_node(std::move(noise));
        graph.add_wire({cid, "grid", nid, "grid"});
        ls::eval_engine engine;
        engine.evaluate(graph, seed);
        return std::get<std::shared_ptr<ls::grid>>(*engine.get_output(nid, "grid"));
    };

    auto g1 = run(0);
    auto g2 = run(999);
    bool any_different = false;
    for (int y = 0; y < 20 && !any_different; ++y)
        for (int x = 0; x < 20 && !any_different; ++x)
            if (g1->get(x, y) != g2->get(x, y))
                any_different = true;
    CHECK(any_different);
}

TEST_CASE("eval cycle detection throws", "[eval]") {
    ls::node_graph graph;
    int a = graph.add_node(std::make_unique<ls::node_noise_grid>());
    int b = graph.add_node(std::make_unique<ls::node_noise_grid>());
    graph.add_wire({a, "grid", b, "grid"});
    graph.add_wire({b, "grid", a, "grid"});
    ls::eval_engine engine;
    CHECK_THROWS_AS(engine.evaluate(graph, 0), std::runtime_error);
}

// ---- generator ----------------------------------------------------------

static ls::generator make_cave_generator() {
    ls::generator gen;
    ls::node_graph& graph = gen.graph();

    auto create = std::make_unique<ls::node_create_grid>();
    create->m_width = 32;
    create->m_height = 32;
    int create_id = graph.add_node(std::move(create));

    auto density_in = std::make_unique<ls::node_input_number>();
    density_in->m_value = 0.45;
    density_in->set_name("density");
    int density_id = graph.add_node(std::move(density_in));

    auto noise = std::make_unique<ls::node_noise_grid>();
    noise->density = 0.45;
    int noise_id = graph.add_node(std::move(noise));

    auto ca = std::make_unique<ls::node_cellular_automata>();
    ca->m_iterations = 3;
    int ca_id = graph.add_node(std::move(ca));

    auto out = std::make_unique<ls::node_output_grid>();
    out->set_name("level");
    int out_id = graph.add_node(std::move(out));

    graph.add_wire({create_id,  "grid",   noise_id,  "grid"    });
    graph.add_wire({density_id, "value",  noise_id,  "density" });
    graph.add_wire({noise_id,   "grid",   ca_id,     "input"   });
    graph.add_wire({ca_id,      "output", out_id,    "value"   });

    gen.rebuild_bindings();
    return gen;
}

TEST_CASE("generator get_grid_output returns a valid grid", "[generator]") {
    auto gen = make_cave_generator();
    gen.set_seed(1);
    gen.evaluate();

    auto grid = gen.get_grid_output("level");
    REQUIRE(grid != nullptr);
    CHECK(grid->width() == 32);
    CHECK(grid->height() == 32);
}

TEST_CASE("generator set_parameter changes output", "[generator]") {
    auto gen1 = make_cave_generator();
    gen1.set_seed(1);
    gen1.set_parameter("density", 0.0); // all zeros -> CA keeps mostly zeros
    gen1.evaluate();
    auto g1 = gen1.get_grid_output("level");

    auto gen2 = make_cave_generator();
    gen2.set_seed(1);
    gen2.set_parameter("density", 1.0); // all ones -> CA keeps mostly ones
    gen2.evaluate();
    auto g2 = gen2.get_grid_output("level");

    // The two outputs should differ
    bool any_different = false;
    for (int y = 0; y < 32 && !any_different; ++y)
        for (int x = 0; x < 32 && !any_different; ++x)
            if (g1->get(x, y) != g2->get(x, y))
                any_different = true;
    CHECK(any_different);
}

TEST_CASE("generator throws on unknown parameter", "[generator]") {
    auto gen = make_cave_generator();
    CHECK_THROWS_AS(gen.set_parameter("nonexistent", 1.0), std::runtime_error);
}

TEST_CASE("generator throws on unknown output", "[generator]") {
    auto gen = make_cave_generator();
    gen.set_seed(0);
    gen.evaluate();
    CHECK_THROWS_AS(gen.get_grid_output("nonexistent"), std::runtime_error);
}

