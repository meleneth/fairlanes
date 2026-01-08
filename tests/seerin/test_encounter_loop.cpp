// encounter_loop.test.cpp

#include <catch2/catch_test_macros.hpp>

#include "sr/bus.hpp"
#include "sr/encounter_events.hpp"
#include "sr/encounter_loop.hpp"
#include "sr/resolver.hpp"

#include <vector>

namespace {
using namespace seerin::enc;

static ns ms(int v) { return std::chrono::milliseconds(v); }

struct RecordingOut {
  std::vector<OutEvent> evs;
};

static bool has_need_command_for(const std::vector<OutEvent> &evs,
                                 CombatantId who) {
  for (const auto &ev : evs) {
    if (std::holds_alternative<NeedCommand>(ev)) {
      if (std::get<NeedCommand>(ev).who == who)
        return true;
    }
  }
  return false;
}

static std::vector<CombatantId>
last_ready_queue(const std::vector<OutEvent> &evs) {
  for (auto it = evs.rbegin(); it != evs.rend(); ++it) {
    if (std::holds_alternative<ReadyQueueChanged>(*it)) {
      return std::get<ReadyQueueChanged>(*it).order;
    }
  }
  return {};
}

struct FakeResolver final : ICombatResolver {
  int calls = 0;
  CombatantId last_who = 0;

  std::vector<InEvent> resolve_action(CombatantId who,
                                      const ResolveContext &) override {
    calls++;
    last_who = who;
    return {}; // no effects for this test
  }
};

} // namespace

TEST_CASE("EncounterLoop mono-active: Beat -> Ready -> NeedCommand -> "
          "CommandChosen -> ActionFinished clears active",
          "[seerin][enc]") {
  seerin::Bus<InEvent> in;
  seerin::Bus<OutEvent> out;

  RecordingOut record;
  auto out_handle =
      out.on([&](const OutEvent &ev) { record.evs.push_back(ev); });

  FakeResolver resolver;

  // Two combatants: 1,2
  std::vector<CombatantId> combatants{
      2, 1}; // intentionally unsorted, encounter sorts for determinism

  seerin::atb::Config cfg{};
  cfg.max_charge_units = 1000;
  cfg.speed_units_per_sec = 1000; // fills in 1 second

  EncounterLoop enc(in, out, resolver, combatants, cfg);

  // Drive 1 second: both should become ready in deterministic order (1 then 2)
  in.emit(InEvent{Beat{ms(1000)}});

  // Encounter should request command for front of ready queue (combatant 1).
  REQUIRE(has_need_command_for(record.evs, 1));
  REQUIRE(enc.active().has_value() == false);
  REQUIRE(enc.awaiting_command().has_value() == true);
  REQUIRE(*enc.awaiting_command() == 1);

  {
    const auto q = last_ready_queue(record.evs);
    REQUIRE(q.size() >= 1);
    REQUIRE(q.front() == 1);
  }

  record.evs.clear();

  // Choose command for combatant 1: 250ms action time
  in.emit(InEvent{CommandChosen{1, ms(250)}});

  REQUIRE(enc.active().has_value());
  REQUIRE(*enc.active() == 1);
  REQUIRE(enc.awaiting_command().has_value() == false);

  record.evs.clear();

  // Drive 250ms: action should finish, encounter clears active and calls
  // resolver.
  in.emit(InEvent{Beat{ms(250)}});

  REQUIRE(enc.active().has_value() == false);
  REQUIRE(resolver.calls == 1);
  REQUIRE(resolver.last_who == 1);

  // Since combatant 2 is still ready, encounter should request command for 2.
  REQUIRE(has_need_command_for(record.evs, 2));
}
