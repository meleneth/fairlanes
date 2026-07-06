#include "fl/skills/skill_sequence.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/field_debuff.hpp"
#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/ecs/systems/combat_status_system.hpp"
#include "fl/ecs/systems/dire_bleed_system.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/damage.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/skills/eviscerate.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/skills/skill_learning.hpp"
#include "fl/skills/skill_visuals.hpp"
#include "fl/skills/thump.hpp"
#include "fl/tracy_shim.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::skills {
namespace {

static constexpr int kFlameWaveExtraHeight = 2;
static constexpr int kHitpointNumberExtraHeight = 2;

int decal_extra_height(fl::widgets::effects::DecalAnimationKind kind) {
  return kind == fl::widgets::effects::DecalAnimationKind::FlameWave
             ? kFlameWaveExtraHeight
             : 0;
}

int thump_rank_tempo_offset(SkillKey skill) noexcept {
  if (skill.base != SkillId::Thump) {
    return 0;
  }

  return skill.rank.value() - SkillRank::kMin;
}

void add_skill_decal(fl::context::PartyCtx &party_ctx, entt::entity target,
                     seerin::uWu expires_at, SkillKey skill) {
  const auto kind = decal_animation_for(skill);
  if (!kind.has_value() || !party_ctx.reg().valid(target)) {
    return;
  }

  fl::ecs::components::add_combatant_decal(
      party_ctx.reg(), target,
      fl::ecs::components::DecalEffect{
          expires_at,
          fl::ecs::components::DecalEffect::Clock::now(),
          std::chrono::seconds{1},
          *kind,
          {},
          decal_extra_height(*kind)});
}

void add_hitpoint_number_decal(fl::context::PartyCtx &party_ctx,
                               entt::entity target, ftxui::Color color,
                               int hitpoints) {
  fl::widgets::effects::DecalConfig config;
  config.color = color;
  config.hitpoints = hitpoints;

  fl::ecs::components::add_combatant_decal(
      party_ctx.reg(), target,
      fl::ecs::components::DecalEffect{
          seerin::uWu{std::numeric_limits<int64_t>::max()},
          fl::ecs::components::DecalEffect::Clock::now(),
          std::chrono::seconds{1},
          fl::widgets::effects::DecalAnimationKind::HitpointNumber, config,
          kHitpointNumberExtraHeight});
}

int apply_damage_skill(fl::context::PartyCtx &party_ctx, entt::entity attacker,
                       entt::entity target, SkillKey skill) {
  fl::skills::Thump thump;
  const int damage = thump.thump(
      fl::context::AttackCtx::make_attack(party_ctx, attacker, target), skill);
  if (damage > 0 && party_ctx.reg().valid(target)) {
    party_ctx.party_data().encounter_data().combatant_bus(target).emit(
        fl::events::CombatantEvent{
            fl::events::SkillHitLanded{attacker, target, skill, damage}});
  }
  return damage;
}

std::vector<entt::entity>
opposing_alive_targets(fl::context::PartyCtx &party_ctx,
                       entt::entity attacker) {
  auto &encounter = party_ctx.party_data().encounter_data();
  return encounter.attackers().contains(attacker)
             ? encounter.defenders().alive_members(party_ctx)
             : encounter.attackers().alive_members(party_ctx);
}

std::vector<entt::entity> allied_alive_targets(fl::context::PartyCtx &party_ctx,
                                               entt::entity actor) {
  auto &encounter = party_ctx.party_data().encounter_data();
  return encounter.attackers().contains(actor)
             ? encounter.attackers().alive_members(party_ctx)
             : encounter.defenders().alive_members(party_ctx);
}

bool is_wired_status_skill(SkillKey skill) noexcept {
  switch (skill.base) {
  case SkillId::ShellGuard:
  case SkillId::ArmorPlate:
  case SkillId::SmokeScreen:
  case SkillId::HushHex:
  case SkillId::Clearbell:
  case SkillId::KindleWound:
  case SkillId::RootLeech:
  case SkillId::WebSnare:
  case SkillId::BlueScreen:
  case SkillId::SignalJam:
  case SkillId::ChecksumWard:
  case SkillId::Choirguard:
  case SkillId::CinderVeil:
  case SkillId::RimeArmor:
  case SkillId::BattleFocus:
  case SkillId::PackHowl:
  case SkillId::SignalFlare:
  case SkillId::ClockUp:
  case SkillId::Burrow:
  case SkillId::GnatCloud:
  case SkillId::MiasmaCloud:
  case SkillId::Whiteout:
  case SkillId::WeightOfTuesday:
  case SkillId::Overcharge:
    return true;
  default:
    return false;
  }
}

fl::ecs::components::FieldTeam
enemy_field_team(fl::context::PartyCtx &party_ctx, entt::entity actor) {
  auto &encounter = party_ctx.party_data().encounter_data();
  return encounter.attackers().contains(actor)
             ? fl::ecs::components::FieldTeam::Defenders
             : fl::ecs::components::FieldTeam::Attackers;
}

void apply_wired_skill_effect(fl::context::PartyCtx &party_ctx,
                              SkillSequencer::Scheduler &scheduler,
                              entt::entity attacker, entt::entity target,
                              SkillKey skill) {
  using fl::ecs::components::CombatStatusKind;
  using fl::ecs::components::FieldDebuffKind;
  using fl::ecs::systems::CombatStatusSystem;

  switch (skill.base) {
  case SkillId::ShellGuard:
    CombatStatusSystem::apply_status(party_ctx, scheduler,
                                     {.kind = CombatStatusKind::Shield,
                                      .name = display_name(skill),
                                      .source = attacker,
                                      .target = target,
                                      .duration_seconds = 30,
                                      .value = 35,
                                      .negative = false});
    return;
  case SkillId::ArmorPlate:
    CombatStatusSystem::apply_status(party_ctx, scheduler,
                                     {.kind = CombatStatusKind::Shield,
                                      .name = display_name(skill),
                                      .source = attacker,
                                      .target = target,
                                      .duration_seconds = 45,
                                      .value = 60,
                                      .negative = false});
    return;
  case SkillId::SmokeScreen:
    CombatStatusSystem::apply_field_debuff(
        party_ctx, scheduler,
        {.team = enemy_field_team(party_ctx, attacker),
         .kind = FieldDebuffKind::AccuracyDown,
         .name = display_name(skill),
         .source = attacker,
         .value = 35,
         .duration_seconds = 30});
    return;
  case SkillId::HushHex:
    CombatStatusSystem::apply_status(party_ctx, scheduler,
                                     {.kind = CombatStatusKind::Silence,
                                      .name = display_name(skill),
                                      .source = attacker,
                                      .target = target,
                                      .duration_seconds = 24});
    return;
  case SkillId::Clearbell:
    CombatStatusSystem::cleanse(party_ctx, attacker, target);
    return;
  case SkillId::KindleWound: {
    apply_damage_skill(party_ctx, attacker, target, skill);
    const bool already_burning = CombatStatusSystem::has_status(
        party_ctx.reg(), target, CombatStatusKind::Burn);
    const auto *stats =
        party_ctx.reg().try_get<fl::ecs::components::Stats>(target);
    const bool wounded = stats != nullptr && stats->hp_ < stats->max_hp_;
    CombatStatusSystem::apply_status(
        party_ctx, scheduler,
        {.kind = CombatStatusKind::Burn,
         .name = display_name(skill),
         .source = attacker,
         .target = target,
         .duration_seconds = 18,
         .tick_damage = already_burning || wounded ? 3 : 2,
         .tick_count = 3});
    return;
  }
  case SkillId::RootLeech: {
    auto damage = fl::primitives::Damage{};
    damage.physical = 6;
    CombatStatusSystem::drain(party_ctx, attacker, target, damage, 50,
                              display_name(skill));
    return;
  }
  case SkillId::WebSnare:
    CombatStatusSystem::apply_status(party_ctx, scheduler,
                                     {.kind = CombatStatusKind::Slow,
                                      .name = display_name(skill),
                                      .source = attacker,
                                      .target = target,
                                      .duration_seconds = 30,
                                      .value = 25});
    return;
  case SkillId::BlueScreen: {
    auto targets = opposing_alive_targets(party_ctx, attacker);
    if (targets.empty() && target != entt::null) {
      targets.push_back(target);
    }
    for (const auto victim : targets) {
      CombatStatusSystem::apply_status(party_ctx, scheduler,
                                       {.kind = CombatStatusKind::Stun,
                                        .name = display_name(skill),
                                        .source = attacker,
                                        .target = victim,
                                        .duration_seconds = 12,
                                        .stacks = 1});
    }
    return;
  }
  case SkillId::SignalJam:
    CombatStatusSystem::apply_field_debuff(
        party_ctx, scheduler,
        {.team = enemy_field_team(party_ctx, attacker),
         .kind = FieldDebuffKind::DamageDown,
         .name = display_name(skill),
         .source = attacker,
         .value = 30,
         .duration_seconds = 30});
    return;
  case SkillId::ChecksumWard:
    CombatStatusSystem::cleanse(party_ctx, attacker, target);
    // TODO: Replace or supplement this with removable-negative-status
    // resistance once a shared ward/resistance mechanic exists.
    CombatStatusSystem::apply_status(party_ctx, scheduler,
                                     {.kind = CombatStatusKind::Shield,
                                      .name = display_name(skill),
                                      .source = attacker,
                                      .target = target,
                                      .duration_seconds = 36,
                                      .value = 45,
                                      .negative = false});
    return;
  case SkillId::Choirguard:
    for (const auto ally : allied_alive_targets(party_ctx, attacker)) {
      CombatStatusSystem::apply_status(party_ctx, scheduler,
                                       {.kind = CombatStatusKind::Shield,
                                        .name = display_name(skill),
                                        .source = attacker,
                                        .target = ally,
                                        .duration_seconds = 30,
                                        .value = 30,
                                        .negative = false});
    }
    return;
  case SkillId::CinderVeil:
    CombatStatusSystem::apply_status(party_ctx, scheduler,
                                     {.kind = CombatStatusKind::Shield,
                                      .name = display_name(skill),
                                      .source = attacker,
                                      .target = target,
                                      .duration_seconds = 30,
                                      .value = 25,
                                      .negative = false});
    return;
  case SkillId::RimeArmor:
    CombatStatusSystem::apply_status(party_ctx, scheduler,
                                     {.kind = CombatStatusKind::Shield,
                                      .name = display_name(skill),
                                      .source = attacker,
                                      .target = target,
                                      .duration_seconds = 36,
                                      .value = 40,
                                      .negative = false});
    return;
  case SkillId::BattleFocus:
    CombatStatusSystem::apply_field_debuff(
        party_ctx, scheduler,
        {.team = enemy_field_team(party_ctx, attacker),
         .kind = FieldDebuffKind::Vulnerable,
         .name = display_name(skill),
         .source = attacker,
         .value = 15,
         .duration_seconds = 24});
    return;
  case SkillId::PackHowl:
    CombatStatusSystem::apply_field_debuff(
        party_ctx, scheduler,
        {.team = enemy_field_team(party_ctx, attacker),
         .kind = FieldDebuffKind::Vulnerable,
         .name = display_name(skill),
         .source = attacker,
         .value = 25,
         .duration_seconds = 24});
    return;
  case SkillId::SignalFlare:
    CombatStatusSystem::apply_field_debuff(
        party_ctx, scheduler,
        {.team = enemy_field_team(party_ctx, attacker),
         .kind = FieldDebuffKind::AccuracyDown,
         .name = display_name(skill),
         .source = attacker,
         .value = 20,
         .duration_seconds = 24});
    return;
  case SkillId::ClockUp:
    CombatStatusSystem::apply_status(party_ctx, scheduler,
                                     {.kind = CombatStatusKind::Haste,
                                      .name = display_name(skill),
                                      .source = attacker,
                                      .target = target,
                                      .duration_seconds = 30,
                                      .value = 25,
                                      .negative = false});
    return;
  case SkillId::Burrow:
    CombatStatusSystem::apply_status(party_ctx, scheduler,
                                     {.kind = CombatStatusKind::Shield,
                                      .name = display_name(skill),
                                      .source = attacker,
                                      .target = attacker,
                                      .duration_seconds = 30,
                                      .value = 45,
                                      .negative = false});
    return;
  case SkillId::GnatCloud:
    CombatStatusSystem::apply_field_debuff(
        party_ctx, scheduler,
        {.team = enemy_field_team(party_ctx, attacker),
         .kind = FieldDebuffKind::AccuracyDown,
         .name = display_name(skill),
         .source = attacker,
         .value = 30,
         .duration_seconds = 24});
    return;
  case SkillId::MiasmaCloud:
    CombatStatusSystem::apply_field_debuff(
        party_ctx, scheduler,
        {.team = enemy_field_team(party_ctx, attacker),
         .kind = FieldDebuffKind::DamageDown,
         .name = display_name(skill),
         .source = attacker,
         .value = 25,
         .duration_seconds = 30});
    return;
  case SkillId::Whiteout:
    CombatStatusSystem::apply_field_debuff(
        party_ctx, scheduler,
        {.team = enemy_field_team(party_ctx, attacker),
         .kind = FieldDebuffKind::AccuracyDown,
         .name = display_name(skill),
         .source = attacker,
         .value = 45,
         .duration_seconds = 30});
    return;
  case SkillId::WeightOfTuesday: {
    const auto targets = opposing_alive_targets(party_ctx, attacker);
    for (const auto victim : targets) {
      CombatStatusSystem::apply_status(party_ctx, scheduler,
                                       {.kind = CombatStatusKind::Slow,
                                        .name = display_name(skill),
                                        .source = attacker,
                                        .target = victim,
                                        .duration_seconds = 30,
                                        .value = 35});
    }
    CombatStatusSystem::apply_field_debuff(
        party_ctx, scheduler,
        {.team = enemy_field_team(party_ctx, attacker),
         .kind = FieldDebuffKind::DamageDown,
         .name = display_name(skill),
         .source = attacker,
         .value = 20,
         .duration_seconds = 30});
    return;
  }
  case SkillId::Overcharge:
    CombatStatusSystem::apply_field_debuff(
        party_ctx, scheduler,
        {.team = enemy_field_team(party_ctx, attacker),
         .kind = FieldDebuffKind::Vulnerable,
         .name = display_name(skill),
         .source = attacker,
         .value = 35,
         .duration_seconds = 18});
    return;
  default:
    return;
  }
}

int apply_healing(fl::context::PartyCtx &party_ctx, entt::entity healer,
                  entt::entity target, SkillKey skill, int heal_amount) {
  if (!party_ctx.reg().valid(target)) {
    return 0;
  }

  auto *target_stats =
      party_ctx.reg().try_get<fl::ecs::components::Stats>(target);
  if (target_stats == nullptr) {
    return 0;
  }

  const int before = target_stats->hp_;
  target_stats->hp_ =
      std::min(target_stats->max_hp_, target_stats->hp_ + heal_amount);
  const int healed = target_stats->hp_ - before;
  add_hitpoint_number_decal(party_ctx, target, fl::lospec500::color_at(15),
                            healed);

  entt::handle healer_h{party_ctx.reg(), healer};
  entt::handle target_h{party_ctx.reg(), target};
  party_ctx.log().append_markup(
      fmt::format("{} used [ability]({}) on {} for [xp]({}) healing",
                  party_ctx.log().name_tag_for(healer_h), display_name(skill),
                  party_ctx.log().name_tag_for(target_h), healed));
  return healed;
}

} // namespace

SkillSequencer::SkillSequencer(fl::context::PartyCtx &party_ctx,
                               Scheduler &scheduler, FinishTurnFn finish_turn)
    : party_ctx_(party_ctx), scheduler_(scheduler),
      finish_turn_(std::move(finish_turn)) {}

void SkillSequencer::schedule(entt::entity attacker, entt::entity target,
                              SkillKey skill) {
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
    if (skill.base == SkillId::Mercyburst) {
      schedule_mercyburst(attacker, target);
      return;
    }
    schedule_decal_strike(attacker, target, skill);
    return;
  case SkillExecutionKind::DamageStrike:
    if (is_wired_status_skill(skill)) {
      schedule_wired_status_effect(attacker, target, skill);
      return;
    }
    schedule_decal_strike(attacker, target, skill);
    return;
  case SkillExecutionKind::GroupDamage:
    schedule_group_damage(attacker, skill);
    return;
  case SkillExecutionKind::SingleHeal:
    schedule_single_heal(attacker, target, skill, 5);
    return;
  case SkillExecutionKind::GroupHeal:
    schedule_group_heal(attacker, skill, 4);
    return;
  case SkillExecutionKind::PlaceholderEffect:
    if (is_wired_status_skill(skill)) {
      schedule_wired_status_effect(attacker, target, skill);
      return;
    }
    schedule_placeholder_effect(attacker, target, skill);
    return;
  case SkillExecutionKind::Flee:
    schedule_flee(attacker, skill);
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
                                         entt::entity target, SkillKey skill) {
  ZoneScopedN("SkillSequencer::schedule_thump_like");
  auto const kRed = fl::lospec500::color_at(4);
  auto const kYellow = fl::lospec500::color_at(14);

  const auto skill_name = display_name(skill);
  const int rank_tempo_offset = thump_rank_tempo_offset(skill);
  const int damage_beat = 26 + (rank_tempo_offset * 2);
  const int finish_beat = 31 + (rank_tempo_offset * 3);

  teach_party_from_observed_skill(party_ctx_, attacker, skill);

  const auto expires_at = seerin::uWu{
      scheduler_.now().v + seerin::UWU_PER_BEAT.v * (finish_beat + 1)};
  add_skill_decal(party_ctx_, target, expires_at, skill);

  schedule_reek_fade(attacker,
                     fmt::format("{}: attacker red pulse #1", skill_name), 10,
                     14, kRed, kRed);

  schedule_reek_fade(attacker,
                     fmt::format("{}: attacker red pulse #2", skill_name), 18,
                     22, kRed, kRed);

  schedule_reek_fade(target, fmt::format("{}: defender yellow hit", skill_name),
                     22, 30, kYellow, kYellow);

  scheduler_.schedule_smelly_in_beats(
      damage_beat, fmt::format("{}: apply damage", skill_name),
      [&party_ctx = party_ctx_, attacker, target, skill] {
        apply_damage_skill(party_ctx, attacker, target, skill);
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats(
      finish_beat, fmt::format("{}: finish", skill_name),
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

  const auto expires_at =
      seerin::uWu{scheduler_.now().v + seerin::UWU_PER_BEAT.v * 36};
  add_skill_decal(party_ctx_, target, expires_at, SkillId::Eviscerate);

  schedule_reek_fade(attacker, "eviscerate: attacker red slash", 8, 18, kRed,
                     kNormalText);
  schedule_reek_fade(target, "eviscerate: defender wound", 20, 32, kYellow,
                     kRed);

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      24, attacker, "eviscerate: apply dire bleed and finish",
      [&party_ctx = party_ctx_, &scheduler = scheduler_, finish_turn, attacker,
       target] {
        fl::skills::Eviscerate eviscerate;
        eviscerate.eviscerate(
            fl::context::AttackCtx::make_attack(party_ctx, attacker, target));
        fl::ecs::components::safe_add_hp_bar_color(party_ctx.reg(), target,
                                                   fl::lospec500::color_at(4));
        fl::ecs::systems::DireBleedSystem::bind_cleanup_and_schedule(
            party_ctx, scheduler, target);
        finish_turn(attacker);
      });

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

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      24, attacker, "poison: apply and finish",
      [&party_ctx = party_ctx_, finish_turn, attacker, target] {
        party_ctx.party_data().encounter_data().combatant_bus(target).emit(
            fl::events::CombatantEvent{
                fl::events::PoisonApplied{attacker, target, 1, 27}});
        finish_turn(attacker);
      });

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

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      24, attacker, "cold snap: apply freeze and finish",
      [&party_ctx = party_ctx_, finish_turn, attacker, target] {
        party_ctx.party_data().encounter_data().combatant_bus(target).emit(
            fl::events::CombatantEvent{
                fl::events::FreezeApplied{attacker, target, 30}});
        finish_turn(attacker);
      });

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

  add_skill_decal(party_ctx_, target, expires_at, SkillId::FlameStrike);

  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats, target, "flame strike: apply damage",
      [&party_ctx = party_ctx_, attacker, target] {
        apply_damage_skill(party_ctx, attacker, target, SkillId::FlameStrike);
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats + 1, attacker, "flame strike: finish",
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_decal_strike(entt::entity attacker,
                                           entt::entity target,
                                           SkillKey skill) {
  ZoneScopedN("SkillSequencer::schedule_decal_strike");
  static constexpr int kAnimationBeats = seerin::BEATS_PER_SEC;

  teach_party_from_observed_skill(party_ctx_, attacker, skill);

  const auto expires_at = seerin::uWu{
      scheduler_.now().v + seerin::UWU_PER_BEAT.v * (kAnimationBeats + 1)};

  add_skill_decal(party_ctx_, target, expires_at, skill);

  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats, target,
      fmt::format("{}: apply damage", display_name(skill)),
      [&party_ctx = party_ctx_, attacker, target, skill] {
        apply_damage_skill(party_ctx, attacker, target, skill);
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats + 1, attacker,
      fmt::format("{}: finish", display_name(skill)),
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_mercyburst(entt::entity attacker,
                                         entt::entity target) {
  ZoneScopedN("SkillSequencer::schedule_mercyburst");
  static constexpr int kAnimationBeats = seerin::BEATS_PER_SEC;
  static constexpr int kHealAmount = 5;

  teach_party_from_observed_skill(party_ctx_, attacker, SkillId::Mercyburst);

  const auto expires_at = seerin::uWu{
      scheduler_.now().v + seerin::UWU_PER_BEAT.v * (kAnimationBeats + 1)};

  add_skill_decal(party_ctx_, target, expires_at, SkillId::Mercyburst);

  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats, target, "Mercyburst: apply healing",
      [&party_ctx = party_ctx_, attacker, target] {
        if (!party_ctx.reg().valid(target)) {
          return;
        }

        auto *target_stats =
            party_ctx.reg().try_get<fl::ecs::components::Stats>(target);
        if (target_stats == nullptr) {
          return;
        }

        const int before = target_stats->hp_;
        target_stats->hp_ =
            std::min(target_stats->max_hp_, target_stats->hp_ + kHealAmount);
        const int healed = target_stats->hp_ - before;
        add_hitpoint_number_decal(party_ctx, target,
                                  fl::lospec500::color_at(15), healed);

        entt::handle attacker_h{party_ctx.reg(), attacker};
        entt::handle target_h{party_ctx.reg(), target};
        party_ctx.log().append_markup(fmt::format(
            "{} used [ability](Mercyburst) on {} for [xp]({}) healing",
            party_ctx.log().name_tag_for(attacker_h),
            party_ctx.log().name_tag_for(target_h), healed));
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats + 1, attacker, "Mercyburst: finish",
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_single_heal(entt::entity attacker,
                                          entt::entity target, SkillKey skill,
                                          int heal_amount) {
  ZoneScopedN("SkillSequencer::schedule_single_heal");
  static constexpr int kAnimationBeats = seerin::BEATS_PER_SEC;

  teach_party_from_observed_skill(party_ctx_, attacker, skill);

  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats, target,
      fmt::format("{}: apply healing", display_name(skill)),
      [&party_ctx = party_ctx_, attacker, target, skill, heal_amount] {
        apply_healing(party_ctx, attacker, target, skill, heal_amount);
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      kAnimationBeats + 1, attacker,
      fmt::format("{}: finish", display_name(skill)),
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_group_heal(entt::entity attacker, SkillKey skill,
                                         int heal_amount) {
  ZoneScopedN("SkillSequencer::schedule_group_heal");
  static constexpr int kAnimationBeats = seerin::BEATS_PER_SEC;
  static constexpr int kStaggerBeats = 2;

  teach_party_from_observed_skill(party_ctx_, attacker, skill);

  const auto targets = allied_alive_targets(party_ctx_, attacker);
  int index = 0;
  for (const auto target : targets) {
    const int heal_beat = (index * kStaggerBeats) + kAnimationBeats;
    scheduler_.schedule_smelly_in_beats_for(
        heal_beat, target,
        fmt::format("{}: heal target {}", display_name(skill), index),
        [&party_ctx = party_ctx_, attacker, target, skill, heal_amount] {
          apply_healing(party_ctx, attacker, target, skill, heal_amount);
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
      finish_beat, attacker, fmt::format("{}: finish", display_name(skill)),
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

          add_skill_decal(party_ctx, target, expires_at, SkillId::FlameWave);
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

          apply_damage_skill(party_ctx, attacker, target, SkillId::FlameWave);
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

void SkillSequencer::schedule_group_damage(entt::entity attacker,
                                           SkillKey skill) {
  ZoneScopedN("SkillSequencer::schedule_group_damage");
  static constexpr int kAnimationBeats = seerin::BEATS_PER_SEC;
  static constexpr int kStaggerBeats = 3;

  teach_party_from_observed_skill(party_ctx_, attacker, skill);

  const auto targets = opposing_alive_targets(party_ctx_, attacker);
  int index = 0;
  for (const auto target : targets) {
    const int start_beat = index * kStaggerBeats;
    const int damage_beat = start_beat + kAnimationBeats;
    const auto expires_at = seerin::uWu{
        scheduler_.now().v + seerin::UWU_PER_BEAT.v * (damage_beat + 1)};

    scheduler_.schedule_smelly_in_beats_for(
        start_beat, target,
        fmt::format("{}: start target {}", display_name(skill), index),
        [&party_ctx = party_ctx_, target, expires_at, skill] {
          if (!party_ctx.reg().valid(target)) {
            return;
          }

          auto *stats =
              party_ctx.reg().try_get<fl::ecs::components::Stats>(target);
          if (stats == nullptr || !stats->is_alive()) {
            return;
          }

          add_skill_decal(party_ctx, target, expires_at, skill);
        });

    scheduler_.schedule_smelly_in_beats_for(
        damage_beat, target,
        fmt::format("{}: damage target {}", display_name(skill), index),
        [&party_ctx = party_ctx_, attacker, target, skill] {
          if (!party_ctx.reg().valid(target)) {
            return;
          }

          auto *stats =
              party_ctx.reg().try_get<fl::ecs::components::Stats>(target);
          if (stats == nullptr || !stats->is_alive()) {
            return;
          }

          apply_damage_skill(party_ctx, attacker, target, skill);
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
      finish_beat, attacker, fmt::format("{}: finish", display_name(skill)),
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_wired_status_effect(entt::entity attacker,
                                                  entt::entity target,
                                                  SkillKey skill) {
  ZoneScopedN("SkillSequencer::schedule_wired_status_effect");
  teach_party_from_observed_skill(party_ctx_, attacker, skill);

  scheduler_.schedule_smelly_in_beats(
      1, fmt::format("{}: apply shared status effect", display_name(skill)),
      [&party_ctx = party_ctx_, &scheduler = scheduler_, attacker, target,
       skill] {
        apply_wired_skill_effect(party_ctx, scheduler, attacker, target, skill);
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats(
      2, fmt::format("{}: finish", display_name(skill)),
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_placeholder_effect(entt::entity attacker,
                                                 entt::entity target,
                                                 SkillKey skill) {
  ZoneScopedN("SkillSequencer::schedule_placeholder_effect");
  teach_party_from_observed_skill(party_ctx_, attacker, skill);

  // TODO: Attach these tagged buff/debuff/cleanse effects to real status
  // systems as those systems become explicit gameplay models.
  scheduler_.schedule_smelly_in_beats(
      1, fmt::format("{}: placeholder effect", display_name(skill)),
      [&party_ctx = party_ctx_, attacker, target, skill] {
        entt::handle attacker_h{party_ctx.reg(), attacker};
        const auto target_name =
            party_ctx.reg().valid(target)
                ? party_ctx.log().name_tag_for(
                      entt::handle{party_ctx.reg(), target})
                : std::string{"the field"};
        party_ctx.log().append_markup(fmt::format(
            "{} used [ability]({}) on {}; its tagged effect is still forming.",
            party_ctx.log().name_tag_for(attacker_h), display_name(skill),
            target_name));
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats(
      2, fmt::format("{}: finish", display_name(skill)),
      [finish_turn, attacker] { finish_turn(attacker); });

  TracyPlot("SkillSequencer.PendingEvents",
            static_cast<double>(scheduler_.pending()));
}

void SkillSequencer::schedule_flee(entt::entity attacker, SkillKey skill) {
  ZoneScopedN("SkillSequencer::schedule_flee");
  const auto &skill_definition = definition(skill);
  const int flee_chance =
      std::clamp(skill_definition.flee_success_percent, 0, 100);
  const auto sub_seq =
      static_cast<std::underlying_type_t<entt::entity>>(attacker);
  auto rs = party_ctx_.rng().stream("encounter/skill/flee", sub_seq);
  const int roll = rs.uniform_int<int>(1, 100);
  const bool success = roll <= flee_chance;

  scheduler_.schedule_smelly_in_beats(
      1, "flee: resolve",
      [&party_ctx = party_ctx_, attacker, flee_chance, roll, success] {
        party_ctx.party_data().encounter_data().combatant_bus(attacker).emit(
            fl::events::CombatantEvent{fl::events::FleeAttempted{
                attacker, flee_chance, roll, success}});

        if (!success || !party_ctx.reg().valid(attacker)) {
          return;
        }

        auto *encounter = party_ctx.party_data().has_encounter()
                              ? &party_ctx.party_data().encounter_data()
                              : nullptr;
        if (encounter == nullptr) {
          return;
        }

        auto *stats =
            party_ctx.reg().try_get<fl::ecs::components::Stats>(attacker);
        if (stats != nullptr) {
          stats->hp_ = 0;
        }

        encounter->clear_active_turn_for(attacker);
        party_ctx.party_data().encounter_data().combatant_bus(attacker).emit(
            fl::events::CombatantEvent{fl::events::CombatantFled{attacker}});
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats(
      2, "flee: finish", [finish_turn, attacker] { finish_turn(attacker); });

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
