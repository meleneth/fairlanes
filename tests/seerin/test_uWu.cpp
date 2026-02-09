#include "sr/uWu.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("uWu is a simple strong type with sane arithmetic and comparisons") {
  using seerin::uWu;

  const uWu a{10};
  const uWu b{25};

  REQUIRE((a + b).v == 35);
  REQUIRE((b - a).v == 15);

  uWu c{5};
  c += uWu{7};
  REQUIRE(c.v == 12);
  c -= uWu{2};
  REQUIRE(c.v == 10);

  REQUIRE(a < b);
  REQUIRE(b > a);
  REQUIRE(uWu{80} == seerin::UWU_PER_BEAT);
}

TEST_CASE("ATB constants: 1 Beat is exactly 80 uWu, expected cadence is 12 "
          "beats/sec") {
  REQUIRE(seerin::UWU_PER_BEAT.v == 80);
  REQUIRE(seerin::BEATS_PER_SEC == 12);

  // Convenience sanity: full charge default will be 4800 => 60 beats
  const seerin::uWu full{4800};
  REQUIRE((full.v / seerin::UWU_PER_BEAT.v) == 60);
}
