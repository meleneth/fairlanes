#include <catch2/catch_test_macros.hpp>

#include "battle_bus_recorder.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/events/beat_bus.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/random_hub.hpp"

TEST_CASE("Encounter:: Encounter innervate wires BattleBus::Tick into "
          "TimedEventQueue",
          "[encounter][wiring]") {
  fl::GrandCentral gc{1, 1, 3};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  fl::events::BeatBus beat_bus;

  fl::primitives::EncounterBuilder builder(party_ctx);

  // You want builder.thump_it_out() to return the encounter entity.
  // If it doesn't, change it. Tests will thank you.
  builder.thump_it_out();

  auto &reg = party_ctx.reg(); // adjust to your actual registry accessor
  auto &enc = reg.get<fl::ecs::components::Encounter>(party_ctx.self());

  // The subject: patch cables in.
  enc.innervate_event_system(beat_bus);

  bool fired = false;

  // Put a timed event into the encounter's timed queue that should fire after
  // 10ms. Adjust API to match your TimedEventQueue.
  enc.timed_events_.schedule_in(std::chrono::milliseconds{10},
                                [&] { fired = true; });

  // Not fired yet
  REQUIRE_FALSE(fired);

  // Drive via bus, not by calling timed_events_ directly.
  enc.battle_bus_.tick(std::chrono::milliseconds{9});
  REQUIRE_FALSE(fired);

  enc.battle_bus_.tick(std::chrono::milliseconds{1});
  REQUIRE(fired);
}
/*

TEST_CASE("Encounter:: Encounter emits battle events on its battle bus",
          "[encounter][bus]") {
  fl::GrandCentral gc{1, 1, 3};

  fl::primitives::RandomHub rng;
  (void)rng; // if not used yet

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);

  auto &reg = party_ctx.reg();
  REQUIRE(reg.valid(party_ctx.self_()));

  auto &enc = reg.get<fl::ecs::components::Encounter>(party_ctx.self_());

  fl::test_support::BattleBusRecorder rec;
  rec.attach(enc.battle_bus_); // adjust accessor: battle_bus(), bus(), etc.

  // Drive the system. Depending on your architecture, do one of these:
  // Option A: encounter methods that emit internally:
  enc.start();
  enc.tick(std::chrono::milliseconds{100});

  // Option B: drive via the bus itself:
  // enc.battle_bus().start_combat(encounter_ent);
  // enc.battle_bus().tick(std::chrono::milliseconds{100});

  auto ids = rec.ids();
  REQUIRE(ids.size() >= 2);
  REQUIRE(ids[0] == fl::events::BattleEventId::StartCombat);
  REQUIRE(ids[1] == fl::events::BattleEventId::Tick);

  // Optional: verify payload contents for StartCombat
  {
    const auto &ev0 = rec.at(0).ev;
    REQUIRE(ev0.id == fl::events::BattleEventId::StartCombat);

    const auto &payload = std::get<fl::events::StartCombat>(ev0.data);
    REQUIRE(payload.encounter == encounter_ent);
  }

  // Optional: verify payload contents for Tick
  {
    const auto &ev1 = rec.at(1).ev;
    const auto &payload = std::get<fl::events::Tick>(ev1.data);
    REQUIRE(payload.dt == std::chrono::milliseconds{100});
  }
}
*/
