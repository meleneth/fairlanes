#include "fl/primitives/encounter_data.hpp"

#include <fmt/format.h>

#include "fl/context.hpp"
#include "fl/ecs/components/color_override.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/monster_identity.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/lospec500.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/skills/eviscerate.hpp"
#include "fl/skills/thump.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "sr/atb_events.hpp"

namespace fl::primitives {
namespace {
constexpr int kDireBleedTickBeats = 36;
}

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

    const auto *monster =
        party_ctx_->reg().try_get<fl::ecs::components::MonsterIdentity>(
            attacker);
    if (monster != nullptr &&
        monster->kind == fl::monster::MonsterKind::HoneyBadger) {
      schedule_eviscerate_sequence(attacker, target);
    } else {
      schedule_thump_sequence(attacker, target);
    }
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

void EncounterData::schedule_eviscerate_sequence(entt::entity attacker,
                                                 entt::entity target) {
  auto const kBg = fl::lospec500::color_at(0);
  auto const kRed = fl::lospec500::color_at(4);
  auto const kYellow = fl::lospec500::color_at(14);

  auto &sched = rt_.atb_.scheduler();

  schedule_reek_fade(attacker, "eviscerate: attacker red slash", 8, 18, kRed,
                     kBg);
  schedule_reek_fade(target, "eviscerate: defender wound", 20, 32, kYellow,
                     kRed);

  sched.schedule_smelly_in_beats_for(
      24, target, "eviscerate: apply dire bleed", [this, attacker, target] {
        fl::skills::Eviscerate eviscerate;
        eviscerate.eviscerate(
            fl::context::AttackCtx::make_attack(*party_ctx_, attacker, target));
        fl::ecs::components::safe_add_hp_bar_color(
            party_ctx_->reg(), target, fl::lospec500::color_at(4));
        bind_dire_bleed_cleanup(target);
        schedule_dire_bleed_tick(target);
      });

  sched.schedule_smelly_in_beats_for(
      34, attacker, "eviscerate: finish", [this, attacker] {
        atb_in().emit(seerin::AtbInEvent{seerin::FinishedTurn{attacker}});
      });
}

void EncounterData::schedule_dire_bleed_tick(entt::entity target) {
  auto &sched = rt_.atb_.scheduler();

  sched.schedule_smelly_in_beats_for(
      kDireBleedTickBeats, target, "dire bleed: tick", [this, target] {
        auto &reg = party_ctx_->reg();
        if (!reg.valid(target)) {
          return;
        }

        auto *bleed = reg.try_get<fl::ecs::components::DireBleed>(target);
        auto *stats = reg.try_get<fl::ecs::components::Stats>(target);
        if (bleed == nullptr || stats == nullptr || !stats->is_alive()) {
          clear_dire_bleed(target);
          return;
        }

        auto attack_ctx = fl::context::AttackCtx::make_attack(
            *party_ctx_, bleed->source, target);
        attack_ctx.damage().physical = bleed->damage_per_tick;
        party_ctx_->log().append_markup(fmt::format(
            "[error](Dire Bleed) tears at [player_name]({}) for [error]({}) "
            "damage.",
            stats->name_, bleed->damage_per_tick));
        fl::ecs::systems::TakeDamage::commit(attack_ctx);

        if (reg.valid(target) &&
            reg.any_of<fl::ecs::components::DireBleed>(target)) {
          schedule_dire_bleed_tick(target);
        }
      });
}

void EncounterData::bind_dire_bleed_cleanup(entt::entity target) {
  auto *bleed =
      party_ctx_->reg().try_get<fl::ecs::components::DireBleed>(target);
  if (bleed == nullptr) {
    return;
  }

  bleed->player_died_sub = fl::events::ScopedPartyListener{
      party_ctx_->bus(), std::in_place_type<fl::events::PlayerDied>,
      [this, target](const fl::events::PlayerDied &ev) {
        if (ev.player == target) {
          clear_dire_bleed(target);
        }
      }};

  bleed->left_combat_sub = fl::events::ScopedPartyListener{
      party_ctx_->bus(), std::in_place_type<fl::events::PartyLeftCombat>,
      [this, target](const fl::events::PartyLeftCombat &) {
        clear_dire_bleed(target);
      }};
}

void EncounterData::clear_dire_bleed(entt::entity target) {
  auto &reg = party_ctx_->reg();
  clear_pending_events_for(target);
  fl::ecs::components::safe_clear_color(reg, target);
  fl::ecs::components::safe_clear_hp_bar_color(reg, target);
  if (reg.valid(target) && reg.any_of<fl::ecs::components::DireBleed>(target)) {
    reg.remove<fl::ecs::components::DireBleed>(target);
  }
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

void EncounterData::clear_pending_events() { rt_.atb_.clear_pending_events(); }

void EncounterData::clear_pending_events_for(entt::entity id) {
  rt_.atb_.clear_pending_events_for(id);
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

    sched.schedule_smelly_in_beats_for(
        beat, entity, fmt::format("{}: reek fade beat {}", label, beat),
        [this, entity, color] {
          fl::ecs::components::safe_add_color(party_ctx_->reg(), entity, color);
        });
  }

  sched.schedule_smelly_in_beats_for(
      end_beat + 1, entity, fmt::format("{}: reek fade clear", label),
      [this, entity] {
        fl::ecs::components::safe_clear_color(party_ctx_->reg(), entity);
      });
}

} // namespace fl::primitives
