#include <fmt/format.h>

#include "encounter_data.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/color_override.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/events/beat_bus.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "sr/atb_events.hpp"

namespace fl::primitives {

void on_encounter_destroy(entt::registry &reg, entt::entity e) {
  auto &enc = reg.get<fl::ecs::components::Encounter>(
      e); // valid: signal fires before removal
  enc.encounter_data().finalize();
}

void install_encounter_hooks(entt::registry &reg) {

  reg.on_destroy<fl::ecs::components::Encounter>()
      .connect<&on_encounter_destroy>();
}

void EncounterData::innervate_event_system() {
  party_ctx_->log().append_markup("innervating event system");

  // atb_in().on<seerin::Beat>([&](const seerin::Beat &) {
  //   party_ctx_->log().append_markup("ATB_IN got Beat");
  // });
  atb_in().on<seerin::FinishedTurn>([&](const seerin::FinishedTurn &) {
    party_ctx_->log().append_markup("ATB_IN got FinishedTurn");
  });

  /*
    beat_tick_handle_ = beat_bus.add_listener(
        fl::events::BeatEventId::Beat, [this](const fl::events::BeatEvent &ev) {
          const auto &beat = std::get<fl::events::BeatPulse>(ev.data).beat;

          battle_bus_.tick(beat.dt);
          timed_events_.advance(beat.dt);
        });
        */
  // --- the important part: wire BecameActive -> schedule thump sequence ---
  atb_out().on<seerin::BecameActive>([this](const seerin::BecameActive &ev) {
    const entt::entity attacker = ev.id;
    party_ctx_->log().append_markup("winding up  thump sequence ");

    const entt::entity target = target_random_alive_opposition(
        attacker); // you said you have this method

    if (target == entt::null)
      return;

    // Pick damage. Hardcode now; later you can route through combat math
    // formulas.

    // Schedule 7 beats worth of effects.
    // Offsets are in beats; tune to taste.
    schedule_thump_sequence(attacker, target);
  });
}

void EncounterData::schedule_thump_sequence(entt::entity attacker,
                                            entt::entity target) {
  constexpr auto kRed = ftxui::Color::Red;
  constexpr auto kYellow = ftxui::Color::Yellow;

  auto &sched = rt_.atb_.scheduler();
  party_ctx_->log().append_markup("queuing thump sequence ");

  // 1: attacker red ON
  sched.schedule_smelly_in(
      seerin::uWu{1}, "thump: attacker red on #1", [this, attacker] {
        fl::ecs::components::safe_add_color(party_ctx_->reg(), attacker, kRed);
      });

  // 2: attacker red OFF
  sched.schedule_smelly_in(
      seerin::uWu{2}, "thump: attacker red off #1", [this, attacker] {
        fl::ecs::components::safe_clear_color(party_ctx_->reg(), attacker);
      });

  // 3: attacker red ON (flash #2)
  sched.schedule_smelly_in(
      seerin::uWu{3}, "thump: attacker red on #2", [this, attacker] {
        fl::ecs::components::safe_add_color(party_ctx_->reg(), attacker, kRed);
      });

  // 4: attacker red OFF
  sched.schedule_smelly_in(
      seerin::uWu{4}, "thump: attacker red off #2", [this, attacker] {
        fl::ecs::components::safe_clear_color(party_ctx_->reg(), attacker);
      });

  // 5: defender yellow ON (longer flash)
  sched.schedule_smelly_in(
      seerin::uWu{5}, "thump: defender yellow on", [this, target] {
        fl::ecs::components::safe_add_color(party_ctx_->reg(), target, kYellow);
      });

  // 6: APPLY DAMAGE mid-yellow
  sched.schedule_smelly_in(
      seerin::uWu{6}, "thump: apply damage", [this, attacker, target] {
        fl::context::AttackCtx::make_attack(*party_ctx_, attacker, target);
      });

  // 7: defender yellow OFF + finish turn
  sched.schedule_smelly_in(
      seerin::uWu{7}, "thump: defender yellow off + finish",
      [this, attacker, target] {
        fl::ecs::components::safe_clear_color(party_ctx_->reg(), target);

        atb_in().emit(seerin::AtbInEvent{seerin::FinishedTurn{attacker}});
      });
}

void EncounterData::finalize() {
  //  ctx_.log().append_markup(
  //    fmt::format("Finalizing encounter with {} entities to clean up",
  //              entities_to_cleanup_.size()));
  /* for (auto e_cleanup : e_to_cleanup_) {
      ctx_.reg_.destroy(e_cleanup);
    }
  */
  // ctx_.log_.append_markup(fmt::format("Encounter {} finalized and cleaned
  // up",
  //                                     int(entt::to_integral(ctx_.self_))));
}

bool EncounterData::has_alive_enemies() {
  using fl::ecs::components::Stats;
  for (auto e : life_.entities_to_cleanup_) {
    if (!party_ctx_->reg().valid(e) || !party_ctx_->reg().all_of<Stats>(e)) {
      continue; // stale or already destroyed, ignore
    }

    auto &enemy = party_ctx_->reg().get<Stats>(e);
    if (enemy.is_alive()) {
      return true;
    }
  }

  return false;
}

bool EncounterData::is_over() {
  // For now, "over" simply means there are no alive enemies.
  return !has_alive_enemies();
}

EncounterData::EncounterData(fl::context::PartyCtx *party_ctx)
    : party_ctx_(party_ctx) {
  wire_.party_beat_ = party_ctx_->bus().appendListener(
      fl::events::PartyEvent::Tick,
      [this](const std::any &) { atb_in().emit(seerin::Beat{}); });

  // atb_in_.on<seerin::Beat>([&](const seerin::Beat &) {
  //   party_ctx_->log().append_markup("ATB_IN got Beat");
  // });

  /* party_tick_tap_ = party_ctx_->bus().appendListener(
        fl::events::PartyEvent::Tick, [this](const std::any &) {
          party_ctx_->log().append_markup(fmt::format(
              "[green](PartyTick) tap TICK {}",
              entt::to_integral(party_ctx_->party_data().party_id())));
        });
  */

  // atb_out_.on<seerin::BecameReady>([&](const seerin::BecameReady &) {
  //  party_ctx_->log().append_markup("BecameReady observed");
  //});

  atb_in().on<seerin::AddCombatant>([this](const seerin::AddCombatant &e) {
    party_ctx_->log().append_markup(
        fmt::format("[magenta](tap) AddCombatant {}", entt::to_integral(e.id)));
  });

  atb_out().on<seerin::BecameReady>([this](auto const &e) {
    party_ctx_->log().append_markup(
        fmt::format("[green](tap) READY {}", entt::to_integral(e.id)));
  });
  atb_out().on<seerin::BecameActive>([this](auto const &e) {
    party_ctx_->log().append_markup(
        fmt::format("[green](tap) ACTIVE {}", entt::to_integral(e.id)));
  });
}
} // namespace fl::primitives
