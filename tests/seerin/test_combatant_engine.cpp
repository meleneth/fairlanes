// combatant_engine.test.cpp

#include <catch2/catch_test_macros.hpp>

#include "sr/atb_events.hpp"
#include "sr/bus.hpp"
#include "sr/combatant_engine.hpp"

#include <vector>

namespace {
using namespace seerin::atb;

static ns ms(int v) { return std::chrono::milliseconds(v); }

struct OutRecorder {
  std::vector<OutputEvent> events;
};

template <typename T> static bool saw(const std::vector<OutputEvent> &evs) {
  for (const auto &ev : evs) {
    bool hit = false;
    std::visit(
        [&](const auto &e) {
          using U = std::decay_t<decltype(e)>;
          if constexpr (std::is_same_v<U, T>)
            hit = true;
        },
        ev);
    if (hit)
      return true;
  }
  return false;
}
} // namespace

TEST_CASE("Charging -> Ready on beats emits BecameReady and StateChanged",
          "[seerin][atb]") {
  seerin::Bus<Event> in;
  seerin::Bus<OutputEvent> out;

  OutRecorder rec;
  auto h = out.on([&](const OutputEvent &e) { rec.events.push_back(e); });

  Config cfg{};
  cfg.max_charge_units = 1000;
  cfg.speed_units_per_sec = 500; // 2 seconds to fill

  CombatantEngine eng(cfg, in, out);

  REQUIRE(eng.is_charging());
  REQUIRE(eng.ctx().charge_units == 0);

  in.emit(Event{Beat{ms(1000)}});
  REQUIRE(eng.is_charging());
  REQUIRE(eng.ctx().charge_units == 500);

  in.emit(Event{Beat{ms(1000)}});
  REQUIRE(eng.is_ready());
  REQUIRE(eng.ctx().charge_units == 1000);

  REQUIRE(saw<BecameReady>(rec.events));
  REQUIRE(saw<StateChanged>(rec.events));
}

TEST_CASE("Ready + CommandSelected -> Acting, Beat finishes -> Charging and "
          "emits ActionFinished",
          "[seerin][atb]") {
  seerin::Bus<Event> in;
  seerin::Bus<OutputEvent> out;

  OutRecorder rec;
  auto h = out.on([&](const OutputEvent &e) { rec.events.push_back(e); });

  Config cfg{};
  cfg.max_charge_units = 1000;
  cfg.speed_units_per_sec = 1000; // 1s to fill
  CombatantEngine eng(cfg, in, out);

  in.emit(Event{Beat{ms(1000)}});
  REQUIRE(eng.is_ready());

  rec.events.clear();
  in.emit(Event{CommandSelected{ms(250)}});
  REQUIRE(eng.is_acting());
  REQUIRE(saw<ActionStarted>(rec.events));

  rec.events.clear();
  in.emit(Event{Beat{ms(250)}});
  REQUIRE(eng.is_charging());
  REQUIRE(eng.ctx().charge_units == 0);
  REQUIRE(saw<ActionFinished>(rec.events));
}

TEST_CASE("Stun pauses progress and returns to correct base state",
          "[seerin][atb]") {
  seerin::Bus<Event> in;
  seerin::Bus<OutputEvent> out;

  Config cfg{};
  cfg.max_charge_units = 1000;
  cfg.speed_units_per_sec = 1000;
  CombatantEngine eng(cfg, in, out);

  // Fill to ready
  in.emit(Event{Beat{ms(1000)}});
  REQUIRE(eng.is_ready());

  // Apply stun
  in.emit(Event{StunApplied{ms(200)}});
  // We don't expose stunned state queries here, but behavior is what matters.

  // Beats during stun should not start acting or change gauge (still full)
  in.emit(Event{Beat{ms(100)}});
  REQUIRE(eng.ctx().charge_units == 1000);

  // End stun
  in.emit(Event{Beat{ms(100)}});
  // back to Ready
  REQUIRE(eng.ctx().charge_units == 1000);

  // Now command works
  in.emit(Event{CommandSelected{ms(50)}});
  REQUIRE(eng.is_acting());
}

TEST_CASE("Killed -> Dead ignores beats; Revived returns Charging or Ready",
          "[seerin][atb]") {
  seerin::Bus<Event> in;
  seerin::Bus<OutputEvent> out;

  Config cfg{};
  cfg.max_charge_units = 1000;
  cfg.speed_units_per_sec = 1000;
  CombatantEngine eng(cfg, in, out);

  in.emit(Event{Beat{ms(500)}});
  REQUIRE(eng.ctx().charge_units > 0);

  in.emit(Event{Killed{}});
  REQUIRE(eng.is_dead());

  // ignored
  in.emit(Event{Beat{ms(1000)}});
  REQUIRE(eng.is_dead());
  REQUIRE(eng.ctx().charge_units == 0);

  // partial revive -> charging
  in.emit(Event{Revived{200}});
  REQUIRE(eng.is_charging());
  REQUIRE(eng.ctx().charge_units == 200);

  // kill + full revive -> ready
  in.emit(Event{Killed{}});
  REQUIRE(eng.is_dead());
  in.emit(Event{Revived{1000}});
  REQUIRE(eng.is_ready());
  REQUIRE(eng.ctx().charge_units == 1000);
}
