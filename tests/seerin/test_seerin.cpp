#include <catch2/catch_test_macros.hpp>

#include "sr/atb_engine.hpp"
#include "sr/encounter_bus.hpp"
#include "sr/encounter_events.hpp"
#include "sr/timed_scheduler.hpp"

namespace seerin::test {

struct Probe {
  int became_ready = 0;
  int apply_effect = 0;

  // Optional: keep payloads if you want deeper assertions later.
  std::vector<seerin::BecameReady> readies;
  std::vector<seerin::ApplyEffect> effects;
};

static void emit_beats(seerin::EncounterBus &bus, int n) {
  for (int i = 0; i < n; ++i)
    bus.emit(seerin::EncounterEvent{seerin::Beat{}});
}

} // namespace seerin::test

TEST_CASE(
    "Core: scheduler fires by beats; ATB emits BecameReady after 60 beats") {
  seerin::EncounterBus bus;
  seerin::test::Probe probe;

  // Record only what we care about, in typed space.
  bus.on<seerin::BecameReady>([&](const seerin::BecameReady &e) {
    ++probe.became_ready;
    probe.readies.push_back(e);
  });

  bus.on<seerin::ApplyEffect>([&](const seerin::ApplyEffect &e) {
    ++probe.apply_effect;
    probe.effects.push_back(e);
  });

  // Timed scheduler emits onto the same bus.
  seerin::TimedScheduler<seerin::EncounterEvent> sched(
      [&](const seerin::EncounterEvent &ev) { bus.emit(ev); });

  // Beat advances scheduler.
  bus.on<seerin::Beat>([&](const seerin::Beat &) { sched.on_beat(); });

  // --- ATB wiring: keep AtbEngine narrow (AtbIn/AtbOut), adapt via
  // EncounterBus.
  seerin::AtbInBus atb_in;
  seerin::AtbOutBus atb_out;

  // Encounter -> ATB
  bus.on<seerin::Beat>([&](const seerin::Beat &e) { atb_in.emit(e); });
  bus.on<seerin::AddCombatant>(
      [&](const seerin::AddCombatant &e) { atb_in.emit(e); });

  // ATB -> Encounter
  atb_out.on<seerin::BecameReady>([&](const seerin::BecameReady &e) {
    bus.emit(seerin::EncounterEvent{e});
  });

  seerin::AtbEngine atb(atb_in, atb_out);
  // ---

  // Add one combatant. (AddCombatant is now just { int id; })
  auto e1 = entt::entity{1};
  bus.emit(seerin::EncounterEvent{seerin::AddCombatant{e1}});

  // Schedule an effect 2 beats later.
  sched.schedule_in_beats(
      2, seerin::EncounterEvent{seerin::ApplyEffect{/*src*/ e1,
                                                    /*dst*/ e1,
                                                    /*skill*/ 42,
                                                    /*magnitude*/ 10}});

  REQUIRE(probe.apply_effect == 0);
  REQUIRE(probe.became_ready == 0);

  // Beat 1: scheduler advances, no effect yet.
  seerin::test::emit_beats(bus, 1);
  REQUIRE(probe.apply_effect == 0);
  REQUIRE(probe.became_ready == 0);

  // Beat 2: scheduled ApplyEffect should fire.
  seerin::test::emit_beats(bus, 1);
  REQUIRE(probe.apply_effect == 1);
  REQUIRE(probe.became_ready == 0);

  // Beats 3..59: still not ready.
  seerin::test::emit_beats(bus, 57);
  REQUIRE(probe.became_ready == 0);

  // Beat 60: should become ready exactly once.
  seerin::test::emit_beats(bus, 1);
  REQUIRE(probe.became_ready == 1);

  // Optional: if you want to assert "which combatant became ready".
  REQUIRE(probe.readies.size() == 1);
  REQUIRE(probe.readies[0].id == e1);
}
