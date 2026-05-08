#include <catch2/catch_test_macros.hpp>
#include <level_synth/tag.hpp>
#include <level_synth/tag_registry.hpp>

using namespace ls;

TEST_CASE("tag default construction", "[tag]") {
    tag empty;
    CHECK(empty.type() == tag_type::symbolic);
    CHECK(empty.raw() == 0);
    CHECK(empty.l0() == 0);
    CHECK(empty.l1() == 0);
    CHECK(empty.l2() == 0);
}

TEST_CASE("tag numeric construction", "[tag]") {
    tag n = tag::numeric(42);
    CHECK(n.type() == tag_type::numeric);
    CHECK(n.value() == 42);

    tag neg = tag::numeric(-1);
    CHECK(neg.type() == tag_type::numeric);
    CHECK(neg.value() == -1);

    tag big = tag::numeric(int64_t(1) << 60);
    CHECK(big.type() == tag_type::numeric);
    CHECK(big.value() == (int64_t(1) << 60));
}

TEST_CASE("tag symbolic construction", "[tag]") {
    tag s = tag::symbolic(100, 200, 300);
    CHECK(s.type() == tag_type::symbolic);
    CHECK(s.l0() == 100);
    CHECK(s.l1() == 200);
    CHECK(s.l2() == 300);
}

TEST_CASE("tag match: symbolic wildcards", "[tag]") {
    tag wall     = tag::symbolic(1, 0, 0);
    tag wall_dmg = tag::symbolic(1, 2, 0);
    tag wall_dmg_l = tag::symbolic(1, 2, 3);
    tag floor    = tag::symbolic(4, 0, 0);

    CHECK(match(wall, wall_dmg));
    CHECK(match(wall_dmg, wall));
    CHECK(match(wall, wall_dmg_l));
    CHECK(match(wall_dmg, wall_dmg_l));
    CHECK_FALSE(match(wall, floor));
}

TEST_CASE("tag match: mixed mode is always false", "[tag]") {
    tag s = tag::symbolic(1, 2, 3);
    tag n = tag::numeric(42);
    CHECK_FALSE(match(s, n));
    CHECK_FALSE(match(n, s));
}

TEST_CASE("tag match: numerics", "[tag]") {
    tag a = tag::numeric(42);
    tag b = tag::numeric(42);
    CHECK(match(a, b));
    CHECK(a == b);

    tag c = tag::numeric(43);
    CHECK_FALSE(match(a, c));
}

TEST_CASE("tag_registry: add and lookup", "[tag]") {
    tag_registry reg;
    CHECK(reg.add("Wall"));
    CHECK(reg.add("Floor"));
    CHECK(reg.add("Level.Wall"));
    CHECK(reg.add("Level.Wall.Damaged"));
    CHECK(reg.add("Level.Wall.Broken"));

    CHECK(reg.find("Wall").has_value());
    CHECK(reg.find("Level.Wall.Damaged").has_value());
    CHECK_FALSE(reg.find("NotDeclared").has_value());

    CHECK(reg.add("Wall")); // re-add is a no-op

    auto t = reg.parse("Level.Wall.Damaged");
    REQUIRE(t.has_value());
    CHECK(reg.identifier(*t) == "Level.Wall.Damaged");

    CHECK(reg.valid(*t));
    CHECK(reg.valid(tag::numeric(123)));
    CHECK_FALSE(reg.valid(tag()));
    CHECK_FALSE(reg.valid(tag::symbolic(99, 99, 99)));
}

TEST_CASE("tag_registry: structural errors", "[tag]") {
    tag_registry reg;
    CHECK_FALSE(reg.add(""));
    CHECK_FALSE(reg.add("."));
    CHECK_FALSE(reg.add("Wall."));
    CHECK_FALSE(reg.add(".Wall"));
    CHECK_FALSE(reg.add("a.b.c.d"));
    CHECK_FALSE(reg.add("a..b"));
}

TEST_CASE("tag_registry: parse independent of registration", "[tag]") {
    tag_registry reg;
    auto t = reg.parse("Random.Unregistered.Tag");
    REQUIRE(t.has_value());
    CHECK(t->type() == tag_type::symbolic);
    CHECK_FALSE(reg.valid(*t));
}

TEST_CASE("tag_registry: parsed and registered tags match", "[tag]") {
    tag_registry reg;
    reg.add("Level.Wall");
    reg.add("Level.Wall.Damaged");
    reg.add("Level.Wall.Broken");

    auto wall = reg.parse("Level.Wall");
    auto dmg  = reg.parse("Level.Wall.Damaged");
    auto brk  = reg.parse("Level.Wall.Broken");
    REQUIRE(wall);
    REQUIRE(dmg);
    REQUIRE(brk);

    CHECK(match(*wall, *dmg));
    CHECK(match(*wall, *brk));
    CHECK_FALSE(match(*dmg, *brk));
}

TEST_CASE("tag_registry: removal", "[tag]") {
    tag_registry reg;
    reg.add("Wall");
    reg.add("Floor");

    CHECK(reg.find("Wall").has_value());
    reg.remove("Wall");
    CHECK_FALSE(reg.find("Wall").has_value());
    CHECK(reg.find("Floor").has_value());
}
