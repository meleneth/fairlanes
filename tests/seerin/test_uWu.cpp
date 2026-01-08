#include <catch2/catch_test_macros.hpp>

#include "sr/uWu.hpp" // adjust include path as needed

using seerin::uWu;

TEST_CASE("uWu starts empty", "[uWu]") {
  uWu gauge{960};

  REQUIRE(gauge.value() == 0);
  REQUIRE_FALSE(gauge.full());
}

TEST_CASE("uWu advances by exactly one tick (80 units)", "[uWu]") {
  uWu gauge{960};

  gauge.tick();

  REQUIRE(gauge.value() == 80);
  REQUIRE_FALSE(gauge.full());
}

TEST_CASE("uWu fills exactly on the final tick", "[uWu]") {
  uWu gauge{160};

  REQUIRE_FALSE(gauge.will_fill_on_tick());
  gauge.tick(); // 80

  REQUIRE(gauge.will_fill_on_tick());
  gauge.tick(); // 160

  REQUIRE(gauge.full());
  REQUIRE(gauge.value() == 160);
}

TEST_CASE("uWu saturates and never exceeds max", "[uWu]") {
  uWu gauge{100};

  gauge.tick(); // +80
  gauge.tick(); // would be +80 again, but should saturate

  REQUIRE(gauge.full());
  REQUIRE(gauge.value() == 100);
}

TEST_CASE("uWu will_fill_on_tick is true when already full", "[uWu]") {
  uWu gauge{80};

  gauge.tick(); // full

  REQUIRE(gauge.full());
  REQUIRE(gauge.will_fill_on_tick());
}

TEST_CASE("uWu tick() is idempotent when full", "[uWu]") {
  uWu gauge{80};

  gauge.tick();
  REQUIRE(gauge.value() == 80);

  gauge.tick();
  gauge.tick();

  REQUIRE(gauge.value() == 80);
}

TEST_CASE("uWu tick_n batches safely", "[uWu]") {
  uWu gauge{960};

  gauge.tick_n(3);

  REQUIRE(gauge.value() == 240);
  REQUIRE_FALSE(gauge.full());
}

TEST_CASE("uWu tick_n saturates without overflow", "[uWu]") {
  uWu gauge{200};

  gauge.tick_n(10); // 10 * 80 = 800, should saturate

  REQUIRE(gauge.full());
  REQUIRE(gauge.value() == 200);
}

TEST_CASE("uWu tick_n with zero or negative ticks is a no-op", "[uWu]") {
  uWu gauge{960};

  gauge.tick_n(0);
  REQUIRE(gauge.value() == 0);

  gauge.tick_n(-5);
  REQUIRE(gauge.value() == 0);
}

TEST_CASE("uWu never overflows when near max", "[uWu][overflow]") {
  uWu gauge{960};

  gauge.tick_n(11); // 880
  REQUIRE(gauge.value() == 880);

  REQUIRE(gauge.will_fill_on_tick());
  gauge.tick(); // should clamp, not overflow

  REQUIRE(gauge.value() == 960);
  REQUIRE(gauge.full());
}
