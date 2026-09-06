#include <catch2/catch_test_macros.hpp>
#include "fl/grand_central.hpp"
#include "fl/ecs/components/stats.hpp"

TEST_CASE("Day zero Visitor consumes its arrival and freezes only the raiding account", "[raid][calendar]") {
  fl::GrandCentral gc{2, 1, 1};
  gc.innervate_event_system();
  auto first = gc.account_context(0);
  auto second = gc.account_context(1);
  auto &account = first.account_data();
  REQUIRE(account.in_raid());
  REQUIRE(second.account_data().in_raid());
  REQUIRE(account.beats_until_visitor() == fl::primitives::AccountData::kVisitorPeriodBeats);
  for (auto id : account.raid()->encounter().defenders())
    gc.reg().get<fl::ecs::components::Stats>(id).hp_ = 0;
  gc.beat_bus().emit(seerin::Beat{});
  REQUIRE_FALSE(account.in_raid());
  REQUIRE(account.calendar().elapsed_beats() == 0);
  gc.beat_bus().emit(seerin::Beat{});
  REQUIRE_FALSE(account.in_raid());
  REQUIRE(account.calendar().elapsed_beats() == 1);
  REQUIRE(second.account_data().calendar().elapsed_beats() == 0);
  REQUIRE(second.account_data().in_raid());
  // Advancing calendar alone avoids simulating 22 hours of ordinary fights.
  while (account.beats_until_visitor() > 1) account.calendar().advance_beat();
  account.advance_beat(first);
  REQUIRE(account.in_raid());
  REQUIRE(account.beats_until_visitor() == fl::primitives::AccountData::kVisitorPeriodBeats);
}

TEST_CASE("Attract progression explicitly excludes Visitor arrivals", "[raid][calendar]") {
  fl::GrandCentral gc{1, 1, 1};
  gc.innervate_event_system(false);
  gc.beat_bus().emit(seerin::Beat{});
  auto &account = gc.account_context(0).account_data();
  REQUIRE_FALSE(account.in_raid());
  REQUIRE(account.calendar().elapsed_beats() == 1);
}
