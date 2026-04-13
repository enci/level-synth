#include <catch2/catch_test_macros.hpp>
#include <level_synth/attribute_grid.hpp>

TEST_CASE("attribute_grid construction", "[attribute_grid]") {
    ls::attribute_grid grid(10, 8);
    CHECK(grid.width() == 10);
    CHECK(grid.height() == 8);
}

TEST_CASE("attribute_grid default construction", "[attribute_grid]") {
    ls::attribute_grid grid;
    CHECK(grid.width() == 0);
    CHECK(grid.height() == 0);
}

TEST_CASE("attribute_grid add and query attributes", "[attribute_grid]") {
    ls::attribute_grid grid(4, 4);

    REQUIRE_FALSE(grid.has_attribute("tile"));
    grid.add_attribute("tile", 1);
    REQUIRE(grid.has_attribute("tile"));

    SECTION("default value fills grid") {
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
                CHECK(grid.get("tile", x, y) == 1);
    }

    SECTION("set and get") {
        grid.set("tile", 2, 3, 42);
        CHECK(grid.get("tile", 2, 3) == 42);
        CHECK(grid.get("tile", 0, 0) == 1);
    }

    SECTION("duplicate add is ignored") {
        grid.set("tile", 0, 0, 99);
        grid.add_attribute("tile", 0);
        CHECK(grid.get("tile", 0, 0) == 99);
    }
}

TEST_CASE("attribute_grid data span", "[attribute_grid]") {
    ls::attribute_grid grid(3, 2);
    grid.add_attribute("val");

    auto span = grid.data("val");
    REQUIRE(span.size() == 6);

    span[0] = 10;
    CHECK(grid.get("val", 0, 0) == 10);
}

TEST_CASE("attribute_grid in_bounds", "[attribute_grid]") {
    ls::attribute_grid grid(5, 5);
    CHECK(grid.in_bounds(0, 0));
    CHECK(grid.in_bounds(4, 4));
    CHECK_FALSE(grid.in_bounds(-1, 0));
    CHECK_FALSE(grid.in_bounds(0, -1));
    CHECK_FALSE(grid.in_bounds(5, 0));
    CHECK_FALSE(grid.in_bounds(0, 5));
}

TEST_CASE("attribute_grid throws on unknown attribute", "[attribute_grid]") {
    ls::attribute_grid grid(2, 2);
    CHECK_THROWS_AS(grid.get("nope", 0, 0), std::runtime_error);
    CHECK_THROWS_AS(grid.set("nope", 0, 0, 1), std::runtime_error);
    CHECK_THROWS_AS(grid.data("nope"), std::runtime_error);
}
