#include "fl/primitives/encounter_data.hpp"

#include <fmt/format.h>

#include "fl/context.hpp"
#include "fl/ecs/components/color_override.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/skills/thump.hpp"
#include "sr/atb_events.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::primitives {

void EncounterData::innervate_event_system() {
  atb_out().on<seerin::BecameActive>([this](const seerin::BecameActive &ev) {
    const entt::entity attacker = ev.id;
    const entt::entity target = target_random_alive_opposition(attacker);

    if (target == entt::null) {
      party_ctx_->log().append_markup(fmt::format(
          "{} did nothing.",
          party_ctx_->reg().get<fl::ecs::components::Stats>(attacker).name_));

      atb_in().emit(seerin::AtbInEvent{seerin::FinishedTurn{attacker}});
      return;
    }

    schedule_thump_sequence(attacker, target);
  });
}

void EncounterData::schedule_thump_sequence(entt::entity attacker,
                                            entt::entity target) {
  constexpr auto kRed = ftxui::Color::Red;
  constexpr auto kYellow = ftxui::Color::Yellow;

  auto &sched = rt_.atb_.scheduler();

  sched.schedule_smelly_in_beats(
      10, "thump: attacker red on #1", [this, attacker] {
        fl::ecs::components::safe_add_color(party_ctx_->reg(), attacker, kRed);
      });

  sched.schedule_smelly_in_beats(
      20, "thump: attacker red off #1", [this, attacker] {
        fl::ecs::components::safe_clear_color(party_ctx_->reg(), attacker);
      });

  sched.schedule_smelly_in_beats(
      30, "thump: attacker red on #2", [this, attacker] {
        fl::ecs::components::safe_add_color(party_ctx_->reg(), attacker, kRed);
      });

  sched.schedule_smelly_in_beats(
      40, "thump: attacker red off #2", [this, attacker] {
        fl::ecs::components::safe_clear_color(party_ctx_->reg(), attacker);
      });

  sched.schedule_smelly_in_beats(
      50, "thump: defender yellow on", [this, target] {
        fl::ecs::components::safe_add_color(party_ctx_->reg(), target, kYellow);
      });

  sched.schedule_smelly_in_beats(
      60, "thump: apply damage", [this, attacker, target] {
        fl::skills::Thump thump;
        thump.thump(
            fl::context::AttackCtx::make_attack(*party_ctx_, attacker, target));
      });

  sched.schedule_smelly_in_beats(
      70, "thump: defender yellow off + finish", [this, attacker, target] {
        fl::ecs::components::safe_clear_color(party_ctx_->reg(), target);
        atb_in().emit(seerin::AtbInEvent{seerin::FinishedTurn{attacker}});
      });
}

void EncounterData::finalize() {
  party_ctx_->log().append_markup(
      fmt::format("Finalizing encounter with {} entities to clean up",
                  life_.entities_to_cleanup_.size()));

  for (auto e_cleanup : life_.entities_to_cleanup_) {
    party_ctx_->reg().destroy(e_cleanup);
  }

  party_ctx_->log().append_markup(
      fmt::format("Encounter {} finalized and cleaned up",
                  int(entt::to_integral(party_ctx_->self()))));
}

bool EncounterData::has_alive_enemies() {
  using fl::ecs::components::Stats;

  for (auto e : life_.entities_to_cleanup_) {
    if (!party_ctx_->reg().valid(e) || !party_ctx_->reg().all_of<Stats>(e)) {
      continue;
    }

    auto &enemy = party_ctx_->reg().get<Stats>(e);
    if (enemy.is_alive()) {
      return true;
    }
  }

  return false;
}

bool EncounterData::is_over() { return !has_alive_enemies(); }

EncounterData::EncounterData(fl::context::PartyCtx *party_ctx)
    : party_ctx_(party_ctx) {
  wire_.party_beat_ = fl::events::ScopedPartyListener{
      party_ctx_->bus(), fl::events::PartyEvent::Tick,
      [this](const std::any &) { atb_in().emit(seerin::Beat{}); }};
}

} // namespace fl::primitives
