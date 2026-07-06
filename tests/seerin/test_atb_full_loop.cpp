#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include <entt/entt.hpp>

#include "fl/ecs/components/atb_charge.hpp"
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
  entt::registry reg;
  seerin::AtbEngine atb{reg};

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

  auto id = reg.create();
  atb.in().emit(seerin::AtbInEvent{seerin::AddCombatant{id}});

  REQUIRE(reg.all_of<fl::ecs::components::AtbCharge>(id));

  emit_beats(atb.in(), 60);

  REQUIRE(became_ready == 1);
  REQUIRE(became_active == 1);
  REQUIRE(atb.active_combatant() == id);
  REQUIRE(atb.ready_queue().empty());
  REQUIRE(reg.get<fl::ecs::components::AtbCharge>(id).charge == 4800);

  // Spend the turn
  atb.in().emit(seerin::AtbInEvent{seerin::FinishedTurn{id}});

  REQUIRE(atb.active_combatant() == entt::entity{});
  REQUIRE(reg.get<fl::ecs::components::AtbCharge>(id).charge == 0);

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

TEST_CASE("ATB: active combatant pauses charge accrual but not scheduler work",
          "[atb][timing]") {
  entt::registry reg;
  seerin::AtbEngine atb{reg};

  auto waiting = reg.create();
  auto active = reg.create();
  atb.in().emit(seerin::AtbInEvent{seerin::AddCombatant{active}});
  atb.in().emit(seerin::AtbInEvent{seerin::AddCombatant{waiting}});

  atb.active_combatant() = active;
  atb.scheduler().schedule_smelly_in_beats(
      2, "test: scheduler advances while active", [&] {
        reg.get<fl::ecs::components::AtbCharge>(active).max_charge = 1234;
      });

  emit_beats(atb.in(), 3);

  REQUIRE(reg.get<fl::ecs::components::AtbCharge>(active).charge == 0);
  REQUIRE(reg.get<fl::ecs::components::AtbCharge>(waiting).charge == 0);
  REQUIRE(reg.get<fl::ecs::components::AtbCharge>(active).max_charge == 1234);
}

TEST_CASE("ATB: unchargeable active combatant does not stall scheduler-only "
          "combat",
          "[atb][timing][active]") {
  entt::registry reg;
  seerin::AtbEngine atb{reg};

  auto active = reg.create();
  auto waiting = reg.create();
  bool active_alive = true;

  atb.set_can_charge_fn([&](entt::entity entity) {
    return entity != active || active_alive;
  });

  atb.in().emit(seerin::AtbInEvent{seerin::AddCombatant{active}});
  atb.in().emit(seerin::AtbInEvent{seerin::AddCombatant{waiting}});
  atb.active_combatant() = active;
  atb.ready_queue().push_back(waiting);

  atb.scheduler().schedule_smelly_in_beats(
      2, "test: active becomes unable to charge", [&] {
        active_alive = false;
      });

  emit_beats(atb.in(), 2);

  REQUIRE(atb.active_combatant() == waiting);
  REQUIRE(atb.ready_queue().empty());
}

TEST_CASE(
    "ATB: dead combatants reset charge and do not accumulate until alive") {
  entt::registry reg;
  seerin::AtbEngine atb{reg};

  auto id = reg.create();
  bool alive = true;
  int became_ready = 0;

  atb.set_can_charge_fn([&](entt::entity e) { return e == id && alive; });
  atb.out().on<seerin::BecameReady>(
      [&](const seerin::BecameReady &) { ++became_ready; });

  atb.in().emit(seerin::AtbInEvent{seerin::AddCombatant{id}});

  emit_beats(atb.in(), 30);
  REQUIRE(reg.get<fl::ecs::components::AtbCharge>(id).charge == 2400);

  alive = false;
  emit_beats(atb.in(), 1);
  REQUIRE(reg.get<fl::ecs::components::AtbCharge>(id).charge == 0);

  alive = true;
  emit_beats(atb.in(), 30);

  REQUIRE(became_ready == 0);

  emit_beats(atb.in(), 30);
  REQUIRE(became_ready == 1);
}

TEST_CASE("ATB: frozen combatants do not accrue charge", "[atb][freeze]") {
  entt::registry reg;
  seerin::AtbEngine atb{reg};

  auto id = reg.create();
  atb.in().emit(seerin::AtbInEvent{seerin::AddCombatant{id}});

  emit_beats(atb.in(), 30);
  REQUIRE(reg.get<fl::ecs::components::AtbCharge>(id).charge == 2400);

  atb.in().emit(seerin::AtbInEvent{seerin::Frozen{id}});
  emit_beats(atb.in(), 10);
  REQUIRE(reg.get<fl::ecs::components::AtbCharge>(id).charge == 2400);

  atb.in().emit(seerin::AtbInEvent{seerin::Thawed{id}});
  emit_beats(atb.in(), 30);

  REQUIRE(reg.get<fl::ecs::components::AtbCharge>(id).charge == 4800);
  REQUIRE(atb.active_combatant() == id);
}

TEST_CASE("ATB: frozen ready combatants leave and re-enter the ready queue",
          "[atb][freeze][ready_queue]") {
  entt::registry reg;
  seerin::AtbEngine atb{reg};

  auto id = reg.create();
  auto sentinel = reg.create();
  atb.in().emit(seerin::AtbInEvent{seerin::AddCombatant{id}});

  atb.ready_queue().push_back(id);
  reg.get<fl::ecs::components::AtbCharge>(id).charge = 4800;
  atb.active_combatant() = sentinel;

  atb.in().emit(seerin::AtbInEvent{seerin::Frozen{id}});
  REQUIRE(std::find(atb.ready_queue().begin(), atb.ready_queue().end(), id) ==
          atb.ready_queue().end());
  REQUIRE(reg.get<fl::ecs::components::AtbCharge>(id).charge == 4800);

  atb.ready_queue().push_back(sentinel);
  atb.in().emit(seerin::AtbInEvent{seerin::Thawed{id}});

  REQUIRE(atb.ready_queue().size() == 2);
  REQUIRE(atb.ready_queue().back() == id);
}

TEST_CASE("ATB: active-turn cleanup removes only callbacks owned by that turn",
          "[atb][lifetime]") {
  entt::registry reg;
  seerin::AtbEngine atb{reg};

  const auto active_owner = reg.create();
  const auto effect_owner = reg.create();
  int active_callbacks = 0;
  int effect_callbacks = 0;

  atb.scheduler().schedule_smelly_in_beats_for(
      1, active_owner, "active-turn callback", [&] { ++active_callbacks; });
  atb.scheduler().schedule_smelly_in_beats_for(
      1, effect_owner, "effect callback", [&] { ++effect_callbacks; });

  atb.active_combatant() = active_owner;
  atb.clear_active_turn_for(active_owner);
  REQUIRE(atb.active_combatant() == entt::entity{});

  atb.scheduler().on_beat();
  REQUIRE(active_callbacks == 0);
  REQUIRE(effect_callbacks == 1);
}
