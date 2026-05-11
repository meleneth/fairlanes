#include <catch2/catch_test_macros.hpp>

#include "fl/events/battle_bus.hpp"

using fl::events::BattleBus;
using fl::events::EndCombatReason;

TEST_CASE("BattleBus dispatches StartCombat, Tick, EndCombat",
          "[events][battle_bus]") {
  BattleBus bus{};

  bool saw_start = false;
  bool saw_tick = false;
  bool saw_end = false;

  entt::entity expected_encounter = static_cast<entt::entity>(123);

  std::chrono::milliseconds seen_dt{0};
  EndCombatReason seen_reason = EndCombatReason::Aborted;

  (void)bus.on<fl::events::StartCombat>([&](const fl::events::StartCombat &ev) {
    saw_start = true;
    REQUIRE(ev.encounter == expected_encounter);
  });

  (void)bus.on<fl::events::BattleTick>(
      [&](const fl::events::BattleTick &ev) {
    saw_tick = true;
    seen_dt = ev.dt;
  });

  (void)bus.on<fl::events::EndCombat>([&](const fl::events::EndCombat &ev) {
    saw_end = true;
    REQUIRE(ev.encounter == expected_encounter);
    seen_reason = ev.reason;
  });

  bus.start_combat(expected_encounter);
  bus.tick(std::chrono::milliseconds{16});
  bus.end_combat(expected_encounter, EndCombatReason::Victory);

  REQUIRE(saw_start);
  REQUIRE(saw_tick);
  REQUIRE(saw_end);
  REQUIRE(seen_dt == std::chrono::milliseconds{16});
  REQUIRE(seen_reason == EndCombatReason::Victory);
}
