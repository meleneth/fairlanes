#include "fl/primitives/encounter_data.hpp"

#include <fmt/format.h>

#include "fl/context.hpp"
#include "fl/ecs/components/color_override.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/lospec500.hpp"
#include "fl/skills/thump.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "sr/atb_events.hpp"

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
  auto const kBg = fl::lospec500::color_at(0);
  auto const kRed = fl::lospec500::color_at(4);
  auto const kYellow = fl::lospec500::color_at(14);

  auto &sched = rt_.atb_.scheduler();

  schedule_reek_fade(attacker, "thump: attacker red pulse #1", 10, 20, kRed,
                     kBg);

  schedule_reek_fade(attacker, "thump: attacker red pulse #2", 30, 40, kRed,
                     kBg);

  schedule_reek_fade(target, "thump: defender yellow hit", 50, 70, kYellow,
                     kBg);

  sched.schedule_smelly_in_beats(
      60, "thump: apply damage", [this, attacker, target] {
        fl::skills::Thump thump;
        thump.thump(
            fl::context::AttackCtx::make_attack(*party_ctx_, attacker, target));
      });

  sched.schedule_smelly_in_beats(71, "thump: finish", [this, attacker] {
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
  rt_.atb_.set_can_charge_fn([this](entt::entity entity) {
    auto *stats = party_ctx_->reg().try_get<fl::ecs::components::Stats>(entity);
    return stats && stats->is_alive();
  });

  wire_.party_beat_ = fl::events::ScopedPartyListener{
      party_ctx_->bus(), std::in_place_type<fl::events::PartyTick>,
      [this](const fl::events::PartyTick &) { atb_in().emit(seerin::Beat{}); }};
}

void EncounterData::schedule_reek_fade(entt::entity entity,
                                       std::string_view label, int start_beat,
                                       int end_beat, ftxui::Color from,
                                       ftxui::Color to) {
  auto &sched = rt_.atb_.scheduler();

  auto const duration = end_beat - start_beat;
  if (duration <= 0) {
    return;
  }

  for (int beat = start_beat; beat <= end_beat; ++beat) {
    auto const t =
        static_cast<float>(beat - start_beat) / static_cast<float>(duration);

    auto const color = ftxui::Color::Interpolate(t, from, to);

    sched.schedule_smelly_in_beats(
        beat, fmt::format("{}: reek fade beat {}", label, beat),
        [this, entity, color] {
          fl::ecs::components::safe_add_color(party_ctx_->reg(), entity, color);
        });
  }

  sched.schedule_smelly_in_beats(
      end_beat + 1, fmt::format("{}: reek fade clear", label), [this, entity] {
        fl::ecs::components::safe_clear_color(party_ctx_->reg(), entity);
      });
}

} // namespace fl::primitives
