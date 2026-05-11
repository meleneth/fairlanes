#include <iostream>

#include <catch2/catch_test_macros.hpp>
#include <entt/entt.hpp>

#include "sr/atb_engine.hpp"
#include "sr/atb_events.hpp"

namespace {

static void emit_beats(seerin::AtbInBus &in, int n) {
  for (int i = 0; i < n; ++i)
    in.emit(seerin::AtbInEvent{seerin::Beat{}});
}

} // namespace

TEST_CASE("ATB: BecameReady enqueues; Beat pumps to BecameActive; FinishedTurn "
          "resets") {
  seerin::AtbEngine atb;

  int became_ready = 0;
  int became_active = 0;
  std::vector<std::string> trace;

  atb.out().on<seerin::BecameReady>([&](const seerin::BecameReady &e) {
    ++became_ready;
    trace.push_back("ready:" + std::to_string((int)e.id));
  });

  atb.out().on<seerin::BecameActive>([&](const seerin::BecameActive &e) {
    ++became_active;
    trace.push_back("active:" + std::to_string((int)e.id));
  });

  auto id = entt::entity{1};
  atb.in().emit(seerin::AtbInEvent{seerin::AddCombatant{id}});

  emit_beats(atb.in(), 60);

  REQUIRE(became_ready == 1);
  REQUIRE(became_active == 1);
  REQUIRE(atb.active_combatant() == id);
  REQUIRE(atb.ready_queue().empty());

  // Spend the turn
  atb.in().emit(seerin::AtbInEvent{seerin::FinishedTurn{id}});

  REQUIRE(atb.active_combatant() == entt::entity{});

  emit_beats(atb.in(), 60);

  REQUIRE(became_ready == 2);
  REQUIRE(became_active == 2);
  REQUIRE(atb.active_combatant() == id);

  // Optional: ordering sanity
  // Expect ready then active each cycle (because pump happens after tick)
  REQUIRE(trace.size() == 4);
  REQUIRE(trace[0].rfind("ready:", 0) == 0);
  REQUIRE(trace[1].rfind("active:", 0) == 0);
  REQUIRE(trace[2].rfind("ready:", 0) == 0);
  REQUIRE(trace[3].rfind("active:", 0) == 0);
}

TEST_CASE("ATB: dead combatants reset charge and do not accumulate until alive") {
  seerin::AtbEngine atb;

  auto id = entt::entity{7};
  bool alive = true;
  int became_ready = 0;

  atb.set_can_charge_fn([&](entt::entity e) { return e == id && alive; });
  atb.out().on<seerin::BecameReady>([&](const seerin::BecameReady &) {
    ++became_ready;
  });

  atb.in().emit(seerin::AtbInEvent{seerin::AddCombatant{id}});

  emit_beats(atb.in(), 30);
  alive = false;
  emit_beats(atb.in(), 1);

  alive = true;
  emit_beats(atb.in(), 30);

  REQUIRE(became_ready == 0);

  emit_beats(atb.in(), 30);
  REQUIRE(became_ready == 1);
}
