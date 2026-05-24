#include "fl/skills/skill_sequence.hpp"

#include <fmt/format.h>

#include <chrono>
#include <memory>
#include <string>

#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/ecs/systems/dire_bleed_system.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/skills/eviscerate.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/skills/skill_learning.hpp"
#include "fl/skills/skill_visuals.hpp"
#include "fl/skills/thump.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "fl/tracy_shim.hpp"

namespace fl::skills {
namespace {

static constexpr int kCombatantDecalWidth = 80;
static constexpr int kCombatantDecalHeight = 8;

std::shared_ptr<const fl::widgets::effects::DecalAnimation>
make_skill_decal(SkillId skill) {
  const auto kind = decal_animation_for(skill);
  if (!kind.has_value()) {
    return nullptr;
  }
  return fl::widgets::effects::make_decal_animation(*kind, kCombatantDecalWidth,
                                                    kCombatantDecalHeight);
}

} // namespace

SkillSequencer::SkillSequencer(fl::context::PartyCtx &party_ctx,
                               Scheduler &scheduler, FinishTurnFn finish_turn,
                               ClearPendingFn clear_pending)
    : party_ctx_(party_ctx), scheduler_(scheduler),
      finish_turn_(std::move(finish_turn)),
      clear_pending_(std::move(clear_pending)) {}

void SkillSequencer::schedule(entt::entity attacker, entt::entity target,
                              SkillId skill) {
  ZoneScopedN("SkillSequencer::schedule");
  const auto &skill_definition = definition(skill);
  switch (skill_definition.execution) {
  case SkillExecutionKind::Eviscerate:
    schedule_eviscerate(attacker, target);
    return;
  case SkillExecutionKind::Poison:
    schedule_poison(attacker, target);
    return;
  case SkillExecutionKind::ColdSnap:
    schedule_cold_snap(attacker, target);
    return;
  case SkillExecutionKind::FlameStrike:
    schedule_flame_strike(attacker, target);
    return;
  case SkillExecutionKind::FlameWave:
    schedule_flame_wave(attacker);
    return;
  case SkillExecutionKind::DecalStrike:
    schedule_decal_strike(attacker, target, skill);
    return;
  case SkillExecutionKind::ThumpLike:
    schedule_thump_like(attacker, target, skill);
    return;
  case SkillExecutionKind::Observe:
    schedule_observe(attacker);
    return;
  }
}

void SkillSequencer::schedule_thump_like(entt::entity attacker,
                                         entt::entity target, SkillId skill) {
  ZoneScopedN("SkillSequencer::schedule_thump_like");
  auto const kNormalText = fl::lospec500::color_at(32);
  auto const kRed = fl::lospec500::color_at(4);
  auto const kYellow = fl::lospec500::color_at(14);

  const auto skill_name = std::string{name(skill)};

  teach_party_from_observed_skill(party_ctx_, attacker, skill);

  schedule_reek_fade(attacker,
                     fmt::format("{}: attacker red pulse #1", skill_name), 10,
                     20, kRed, kNormalText);

  schedule_reek_fade(attacker,
                     fmt::format("{}: attacker red pulse #2", skill_name), 30,
                     40, kRed, kNormalText);

  schedule_reek_fade(target, fmt::format("{}: defender yellow hit", skill_name),
                     50, 70, kYellow, kNormalText);

  scheduler_.schedule_smelly_in_beats(
      60, fmt::format("{}: apply damage", skill_name),
      [&party_ctx = party_ctx_, attacker, target, skill] {
        fl::skills::Thump thump;
        thump.thump(
            fl::context::AttackCtx::make_attack(party_ctx, attacker, target),
            skill);
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats(
      71, fmt::format("{}: finish", skill_name),
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_eviscerate(entt::entity attacker,
                                         entt::entity target) {
  ZoneScopedN("SkillSequencer::schedule_eviscerate");
  auto const kNormalText = fl::lospec500::color_at(32);
  auto const kRed = fl::lospec500::color_at(4);
  auto const kYellow = fl::lospec500::color_at(14);

  teach_party_from_observed_skill(party_ctx_, attacker, SkillId::Eviscerate);

  schedule_reek_fade(attacker, "eviscerate: attacker red slash", 8, 18, kRed,
                     kNormalText);
  schedule_reek_fade(target, "eviscerate: defender wound", 20, 32, kYellow,
                     kRed);

  scheduler_.schedule_smelly_in_beats_for(
      24, target, "eviscerate: apply dire bleed",
      [&party_ctx = party_ctx_, &scheduler = scheduler_,
       clear_pending = clear_pending_, attacker, target] {
        fl::skills::Eviscerate eviscerate;
        eviscerate.eviscerate(
            fl::context::AttackCtx::make_attack(party_ctx, attacker, target));
        fl::ecs::components::safe_add_hp_bar_color(party_ctx.reg(), target,
                                                   fl::lospec500::color_at(4));
        fl::ecs::systems::DireBleedSystem::bind_cleanup_and_schedule(
            party_ctx, scheduler, target, clear_pending);
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      34, attacker, "eviscerate: finish",
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_poison(entt::entity attacker,
                                     entt::entity target) {
  ZoneScopedN("SkillSequencer::schedule_poison");
  auto const kNormalText = fl::lospec500::color_at(32);
  auto const kGreen = fl::lospec500::color_at(22);

  teach_party_from_observed_skill(party_ctx_, attacker, SkillId::Poison);

  schedule_reek_fade(attacker, "poison: attacker green pulse", 8, 18, kGreen,
                     kNormalText);
  schedule_reek_fade(target, "poison: defender sickly tint", 20, 32, kGreen,
                     kNormalText);

  scheduler_.schedule_smelly_in_beats_for(
      24, target, "poison: apply", [&party_ctx = party_ctx_, attacker, target] {
        party_ctx.bus().emit(fl::events::PartyEvent{
            fl::events::PoisonApplied{attacker, target, 1, 9}});
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      34, attacker, "poison: finish",
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_cold_snap(entt::entity attacker,
                                        entt::entity target) {
  ZoneScopedN("SkillSequencer::schedule_cold_snap");
  auto const kNormalText = fl::lospec500::color_at(32);
  auto const kIce = fl::lospec500::color_at(28);

  teach_party_from_observed_skill(party_ctx_, attacker, SkillId::ColdSnap);

  schedule_reek_fade(attacker, "cold snap: attacker ice pulse", 8, 18, kIce,
                     kNormalText);

  scheduler_.schedule_smelly_in_beats_for(
      24, target, "cold snap: apply freeze",
      [&party_ctx = party_ctx_, attacker, target] {
        party_ctx.bus().emit(fl::events::PartyEvent{
            fl::events::FreezeApplied{attacker, target, 9}});
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      34, attacker, "cold snap: finish",
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_flame_strike(entt::entity attacker,
                                           entt::entity target) {
  ZoneScopedN("SkillSequencer::schedule_flame_strike");
  static constexpr int kAnimationBeats = seerin::BEATS_PER_SEC;

  teach_party_from_observed_skill(party_ctx_, attacker, SkillId::FlameStrike);

  const auto expires_at = seerin::uWu{
      scheduler_.now().v + seerin::UWU_PER_BEAT.v * (kAnimationBeats + 1)};

  if (party_ctx_.reg().valid(target)) {
    auto decal = make_skill_decal(SkillId::FlameStrike);
    if (decal == nullptr) {
      return;
    }

    party_ctx_.reg().emplace_or_replace<fl::ecs::components::FlameWaveDecal>(
        target,
        fl::ecs::components::FlameWaveDecal{
            expires_at, fl::ecs::components::FlameWaveDecal::Clock::now(),
            std::chrono::seconds{1}, std::move(decal)});
  }

  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats, target, "flame strike: apply damage",
      [&party_ctx = party_ctx_, attacker, target] {
        fl::skills::Thump thump;
        thump.thump(
            fl::context::AttackCtx::make_attack(party_ctx, attacker, target),
            SkillId::FlameStrike);
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats + 1, attacker, "flame strike: finish",
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_decal_strike(entt::entity attacker,
                                           entt::entity target, SkillId skill) {
  ZoneScopedN("SkillSequencer::schedule_decal_strike");
  static constexpr int kAnimationBeats = seerin::BEATS_PER_SEC;

  teach_party_from_observed_skill(party_ctx_, attacker, skill);

  const auto expires_at = seerin::uWu{
      scheduler_.now().v + seerin::UWU_PER_BEAT.v * (kAnimationBeats + 1)};

  if (party_ctx_.reg().valid(target)) {
    auto decal = make_skill_decal(skill);
    if (decal == nullptr) {
      return;
    }

    party_ctx_.reg().emplace_or_replace<fl::ecs::components::FlameWaveDecal>(
        target,
        fl::ecs::components::FlameWaveDecal{
            expires_at, fl::ecs::components::FlameWaveDecal::Clock::now(),
            std::chrono::seconds{1}, std::move(decal)});
  }

  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats, target, fmt::format("{}: apply damage", name(skill)),
      [&party_ctx = party_ctx_, attacker, target, skill] {
        fl::skills::Thump thump;
        thump.thump(
            fl::context::AttackCtx::make_attack(party_ctx, attacker, target),
            skill);
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats + 1, attacker, fmt::format("{}: finish", name(skill)),
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_flame_wave(entt::entity attacker) {
  ZoneScopedN("SkillSequencer::schedule_flame_wave");
  static constexpr int kAnimationBeats = seerin::BEATS_PER_SEC;
  static constexpr int kStaggerBeats = 3;

  teach_party_from_observed_skill(party_ctx_, attacker, SkillId::FlameWave);

  auto &encounter = party_ctx_.party_data().encounter_data();
  const auto targets = encounter.attackers().contains(attacker)
                           ? encounter.defenders().alive_members(party_ctx_)
                           : encounter.attackers().alive_members(party_ctx_);

  int index = 0;
  for (const auto target : targets) {
    const int start_beat = index * kStaggerBeats;
    const int damage_beat = start_beat + kAnimationBeats;
    const auto expires_at = seerin::uWu{
        scheduler_.now().v + seerin::UWU_PER_BEAT.v * (damage_beat + 1)};

    scheduler_.schedule_smelly_in_beats_for(
        start_beat, target, fmt::format("flame wave: start target {}", index),
        [&party_ctx = party_ctx_, target, expires_at] {
          if (!party_ctx.reg().valid(target)) {
            return;
          }

          auto *stats =
              party_ctx.reg().try_get<fl::ecs::components::Stats>(target);
          if (stats == nullptr || !stats->is_alive()) {
            return;
          }

          auto decal = make_skill_decal(SkillId::FlameWave);
          if (decal == nullptr) {
            return;
          }

          party_ctx.reg()
              .emplace_or_replace<fl::ecs::components::FlameWaveDecal>(
                  target, fl::ecs::components::FlameWaveDecal{
                              expires_at,
                              fl::ecs::components::FlameWaveDecal::Clock::now(),
                              std::chrono::seconds{1}, std::move(decal)});
        });

    scheduler_.schedule_smelly_in_beats_for(
        damage_beat, target, fmt::format("flame wave: damage target {}", index),
        [&party_ctx = party_ctx_, attacker, target] {
          if (!party_ctx.reg().valid(target)) {
            return;
          }

          auto *stats =
              party_ctx.reg().try_get<fl::ecs::components::Stats>(target);
          if (stats == nullptr || !stats->is_alive()) {
            return;
          }

          fl::skills::Thump thump;
          thump.thump(
              fl::context::AttackCtx::make_attack(party_ctx, attacker, target),
              SkillId::FlameWave);
        });

    ++index;
  }

  auto finish_turn = finish_turn_;
  const int finish_beat =
      targets.empty()
          ? 1
          : ((static_cast<int>(targets.size()) - 1) * kStaggerBeats) +
                kAnimationBeats + 1;
  scheduler_.schedule_smelly_in_beats_for(
      finish_beat, attacker, "flame wave: finish",
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_observe(entt::entity attacker) {
  ZoneScopedN("SkillSequencer::schedule_observe");
  scheduler_.schedule_smelly_in_beats(
      1, "observe: log", [&party_ctx = party_ctx_, attacker] {
        auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(attacker);
        party_ctx.log().append_markup(fmt::format(
            "[player_name]({}) used [ability](Observe).", stats.name_));
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats(
      12, "observe: finish",
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_reek_fade(entt::entity entity,
                                        std::string_view label, int start_beat,
                                        int end_beat, ftxui::Color from,
                                        ftxui::Color to) {
  ZoneScopedN("SkillSequencer::schedule_reek_fade");
  auto const duration = end_beat - start_beat;
  if (duration <= 0) {
    return;
  }

  const auto expires_at =
      seerin::uWu{scheduler_.now().v +
                  seerin::UWU_PER_BEAT.v * static_cast<int64_t>(end_beat + 1)};

  for (int beat = start_beat; beat < end_beat; ++beat) {
    auto const t =
        static_cast<float>(beat - start_beat) / static_cast<float>(duration);

    auto const color = ftxui::Color::Interpolate(t, from, to);

    scheduler_.schedule_smelly_in_beats_for(
        beat, entity, fmt::format("{}: damage flash beat {}", label, beat),
        [&party_ctx = party_ctx_, entity, color, expires_at] {
          if (!party_ctx.reg().valid(entity)) {
            return;
          }
          party_ctx.reg().emplace_or_replace<fl::ecs::components::DamageFlash>(
              entity, fl::ecs::components::DamageFlash{color, expires_at});
        });
  }

  scheduler_.schedule_smelly_in_beats_for(
      end_beat, entity, fmt::format("{}: damage flash release", label),
      [&party_ctx = party_ctx_, entity] {
        if (!party_ctx.reg().valid(entity)) {
          return;
        }
        party_ctx.reg().remove<fl::ecs::components::DamageFlash>(entity);
      });
}

} // namespace fl::skills
