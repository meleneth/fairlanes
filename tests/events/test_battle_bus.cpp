#include <catch2/catch_test_macros.hpp>

#include "fl/events/battle_bus.hpp"

using fl::events::BattleBus;
using fl::events::BattleEventId;
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

  (void)bus.add_listener(
      BattleEventId::StartCombat, [&](const fl::events::BattleEvent &ev) {
        saw_start = true;
        REQUIRE(ev.id == BattleEventId::StartCombat);
        auto payload = std::get<fl::events::StartCombat>(ev.data);
        REQUIRE(payload.encounter == expected_encounter);
      });

  (void)bus.add_listener(BattleEventId::Tick,
                         [&](const fl::events::BattleEvent &ev) {
                           saw_tick = true;
                           REQUIRE(ev.id == BattleEventId::Tick);
                           auto payload = std::get<fl::events::Tick>(ev.data);
                           seen_dt = payload.dt;
                         });

  (void)bus.add_listener(
      BattleEventId::EndCombat, [&](const fl::events::BattleEvent &ev) {
        saw_end = true;
        REQUIRE(ev.id == BattleEventId::EndCombat);
        auto payload = std::get<fl::events::EndCombat>(ev.data);
        REQUIRE(payload.encounter == expected_encounter);
        seen_reason = payload.reason;
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
