#include "fl/ecs/systems/combat_status_system.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <cstdint>
#include <optional>
#include <string>

#include "fl/context.hpp"
#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/field_debuff.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/dire_bleed_system.hpp"
#include "fl/ecs/systems/freeze_system.hpp"
#include "fl/ecs/systems/poison_system.hpp"
#include "fl/ecs/systems/status_effect_lifetime.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {
namespace {
using fl::ecs::components::CombatStatusEffect;
using fl::ecs::components::CombatStatusKind;
using fl::ecs::components::CombatStatuses;
using fl::ecs::components::FieldDebuffKind;
using fl::ecs::components::FieldDebuffs;
using fl::ecs::components::FieldTeam;
using fl::ecs::components::Stats;

constexpr int kBurnTickSeconds = 3;
constexpr int kBurnTickBeats =
    fl::primitives::WorldClock::beats_from_seconds(kBurnTickSeconds);

std::string_view default_status_name(CombatStatusKind kind) {
  switch (kind) {
  case CombatStatusKind::Shield:
    return "Shield";
  case CombatStatusKind::Blind:
    return "Blind";
  case CombatStatusKind::Silence:
    return "Silence";
  case CombatStatusKind::Slow:
    return "Slow";
  case CombatStatusKind::Stun:
    return "Stun";
  case CombatStatusKind::Haste:
    return "Haste";
  case CombatStatusKind::Burn:
    return "Burn";
  }
  return "Status";
}

bool is_positive(CombatStatusKind kind) {
  return kind == CombatStatusKind::Shield || kind == CombatStatusKind::Haste;
}

CombatStatusEffect *find_status(entt::registry &reg, entt::entity target,
                                CombatStatusKind kind) {
  auto *statuses = reg.try_get<CombatStatuses>(target);
  if (statuses == nullptr) {
    return nullptr;
  }

  auto it = std::find_if(statuses->effects.begin(), statuses->effects.end(),
                         [kind](const CombatStatusEffect &effect) {
                           return effect.kind == kind;
                         });
  return it == statuses->effects.end() ? nullptr : &*it;
}

CombatStatusEffect *find_status_by_id(entt::registry &reg, entt::entity target,
                                      int status_id) {
  auto *statuses = reg.try_get<CombatStatuses>(target);
  if (statuses == nullptr) {
    return nullptr;
  }

  auto it = std::find_if(statuses->effects.begin(), statuses->effects.end(),
                         [status_id](const CombatStatusEffect &effect) {
                           return effect.id == status_id;
                         });
  return it == statuses->effects.end() ? nullptr : &*it;
}

void scale_damage(fl::primitives::Damage &damage, int percent) {
  percent = std::max(0, percent);
  damage.physical = damage.physical * percent / 100;
  damage.magical = damage.magical * percent / 100;
  damage.fire = damage.fire * percent / 100;
  damage.ice = damage.ice * percent / 100;
  damage.lightning = damage.lightning * percent / 100;
}

int total_damage(const fl::primitives::Damage &damage) {
  return damage.physical + damage.magical + damage.fire + damage.ice +
         damage.lightning;
}

std::optional<fl::context::PartyCtx> party_ctx_for_combatant(
    entt::registry &reg, entt::entity first, entt::entity second) {
  if (auto *member = reg.try_get<fl::ecs::components::PartyMember>(first)) {
    return member->party().party_data().party_ctx();
  }
  if (auto *member = reg.try_get<fl::ecs::components::PartyMember>(second)) {
    return member->party().party_data().party_ctx();
  }
  return std::nullopt;
}

std::optional<FieldTeam> team_for(fl::context::PartyCtx &party_ctx,
                                  entt::entity entity) {
  if (!party_ctx.party_data().has_encounter()) {
    return std::nullopt;
  }

  auto &encounter = party_ctx.party_data().encounter_data();
  if (encounter.attackers().contains(entity)) {
    return FieldTeam::Attackers;
  }
  if (encounter.defenders().contains(entity)) {
    return FieldTeam::Defenders;
  }
  return std::nullopt;
}

std::string name_for(fl::context::PartyCtx &party_ctx, entt::entity entity) {
  return std::string{party_ctx.log().name_tag_for(
      entt::handle{party_ctx.reg(), entity})};
}

} // namespace

bool CombatStatusSystem::apply_status(fl::context::PartyCtx &party_ctx,
                                      Scheduler &scheduler,
                                      const ApplyStatusRequest &request) {
  auto &reg = party_ctx.reg();
  if (!reg.valid(request.source) || !reg.valid(request.target)) {
    return false;
  }

  auto *stats = reg.try_get<Stats>(request.target);
  if (stats == nullptr || !stats->is_alive()) {
    return false;
  }

  (void)clear_status(party_ctx, request.target, request.kind);

  auto &statuses = reg.get_or_emplace<CombatStatuses>(request.target);
  auto effect = CombatStatusEffect{};
  effect.id = statuses.next_id++;
  effect.kind = request.kind;
  effect.name = request.name.empty() ? std::string{default_status_name(request.kind)}
                                     : std::string{request.name};
  effect.source = request.source;
  effect.value = request.value;
  effect.stacks = std::max(1, request.stacks);
  effect.turns_remaining = request.turns;
  effect.tick_damage = request.tick_damage;
  effect.ticks_remaining = request.tick_count;
  effect.negative = request.negative && !is_positive(request.kind);
  effect.removable = request.removable;
  effect.effect = StatusEffectLifetime::create_instance(party_ctx, request.target);
  const int status_id = effect.id;
  statuses.effects.push_back(std::move(effect));

  auto *stored = find_status_by_id(reg, request.target, status_id);
  if (stored == nullptr) {
    return false;
  }

  StatusEffectLifetime lifetime{party_ctx, scheduler, stored->effect};
  lifetime.on_owner_died([&party_ctx, target = request.target](const fl::events::PlayerDied &) {
    CombatStatusSystem::clear_status_by_id(party_ctx, target, -1);
  });
  lifetime.on_combat_removed([&party_ctx, target = request.target](const auto &) {
    CombatStatusSystem::clear_status_by_id(party_ctx, target, -1);
  });

  if (request.duration_seconds > 0) {
    const int clear_after_beats = fl::primitives::WorldClock::beats_from_seconds(
        std::max(1, request.duration_seconds));
    lifetime.schedule_in_beats(clear_after_beats, "combat status: auto clear",
                               [&party_ctx, target = request.target, status_id] {
                                 CombatStatusSystem::clear_status_by_id(
                                     party_ctx, target, status_id);
                               });
  }

  party_ctx.log().append_markup(fmt::format(
      "{} applied [ability]({}) to {}.", name_for(party_ctx, request.source),
      stored->name, name_for(party_ctx, request.target)));

  if (request.kind == CombatStatusKind::Burn && request.tick_damage > 0 &&
      request.tick_count > 0) {
    schedule_burn_tick(party_ctx, scheduler, request.target, status_id);
  }

  return true;
}

void CombatStatusSystem::clear_status_by_id(fl::context::PartyCtx &party_ctx,
                                            entt::entity target,
                                            int status_id) {
  auto &reg = party_ctx.reg();
  if (!reg.valid(target)) {
    return;
  }

  auto *statuses = reg.try_get<CombatStatuses>(target);
  if (statuses == nullptr) {
    return;
  }

  auto remove_one = [&](CombatStatusEffect &effect) {
    auto &scheduler = party_ctx.party_data().encounter_data().atb_engine().scheduler();
    StatusEffectLifetime lifetime{party_ctx, scheduler, effect.effect};
    lifetime.clear_scheduled();
    lifetime.destroy_instance_entity();
  };

  if (status_id < 0) {
    for (auto &effect : statuses->effects) {
      remove_one(effect);
    }
    reg.remove<CombatStatuses>(target);
    return;
  }

  auto it = std::find_if(statuses->effects.begin(), statuses->effects.end(),
                         [status_id](const CombatStatusEffect &effect) {
                           return effect.id == status_id;
                         });
  if (it == statuses->effects.end()) {
    return;
  }

  remove_one(*it);
  statuses->effects.erase(it);
  if (statuses->effects.empty()) {
    reg.remove<CombatStatuses>(target);
  }
}

bool CombatStatusSystem::clear_status(fl::context::PartyCtx &party_ctx,
                                      entt::entity target,
                                      CombatStatusKind kind) {
  auto &reg = party_ctx.reg();
  auto *effect = find_status(reg, target, kind);
  if (effect == nullptr) {
    return false;
  }

  const int status_id = effect->id;
  clear_status_by_id(party_ctx, target, status_id);
  return true;
}

bool CombatStatusSystem::has_status(entt::registry &reg, entt::entity target,
                                    CombatStatusKind kind) {
  return find_status(reg, target, kind) != nullptr;
}

int CombatStatusSystem::status_value(entt::registry &reg, entt::entity target,
                                     CombatStatusKind kind) {
  const auto *effect = find_status(reg, target, kind);
  return effect == nullptr ? 0 : effect->value;
}

bool CombatStatusSystem::can_use_skill(entt::registry &reg, entt::entity actor,
                                       fl::skills::SkillKey skill) {
  if (!skill.rank.valid()) {
    return false;
  }

  if (has_status(reg, actor, CombatStatusKind::Silence) &&
      fl::skills::has_tag(skill, fl::skills::SkillTag::Spell)) {
    return false;
  }

  return true;
}

bool CombatStatusSystem::consume_stun_turn(fl::context::PartyCtx &party_ctx,
                                           entt::entity actor) {
  auto &reg = party_ctx.reg();
  auto *stun = find_status(reg, actor, CombatStatusKind::Stun);
  if (stun == nullptr) {
    return false;
  }

  party_ctx.log().append_markup(fmt::format(
      "{} is [warn](stunned) and misses a turn.", name_for(party_ctx, actor)));
  --stun->stacks;
  const int status_id = stun->id;
  if (stun->stacks <= 0) {
    clear_status_by_id(party_ctx, actor, status_id);
  }
  return true;
}

bool CombatStatusSystem::attack_misses(fl::context::AttackCtx &ctx) {
  int miss_chance = status_value(ctx.reg(), ctx.attacker(), CombatStatusKind::Blind);
  if (auto party_ctx =
          party_ctx_for_combatant(ctx.reg(), ctx.attacker(), ctx.defender())) {
    if (const auto team = team_for(*party_ctx, ctx.attacker())) {
      miss_chance = std::max(
          miss_chance,
          field_debuff_value(*party_ctx, *team, FieldDebuffKind::AccuracyDown));
    }
  }

  miss_chance = std::clamp(miss_chance, 0, 100);
  if (miss_chance <= 0) {
    return false;
  }

  const auto attacker_id = static_cast<std::uint64_t>(entt::to_integral(ctx.attacker()));
  const auto defender_id = static_cast<std::uint64_t>(entt::to_integral(ctx.defender()));
  auto rs = ctx.rng().stream("combat/status/miss", (attacker_id << 32U) ^ defender_id);
  const int roll = rs.uniform_int<int>(1, 100);
  if (roll > miss_chance) {
    return false;
  }

  ctx.log().append_markup(fmt::format(
      "{} misses {}.",
      ctx.log().name_tag_for(entt::handle{ctx.reg(), ctx.attacker()}),
      ctx.log().name_tag_for(entt::handle{ctx.reg(), ctx.defender()})));
  return true;
}

void CombatStatusSystem::apply_damage_modifiers(fl::context::AttackCtx &ctx,
                                                fl::primitives::Damage &damage) {
  if (auto party_ctx =
          party_ctx_for_combatant(ctx.reg(), ctx.attacker(), ctx.defender())) {
    if (const auto attacker_team = team_for(*party_ctx, ctx.attacker())) {
      const int reduction = field_debuff_value(*party_ctx, *attacker_team,
                                               FieldDebuffKind::DamageDown);
      if (reduction > 0) {
        scale_damage(damage, std::max(0, 100 - reduction));
      }
    }

    if (const auto defender_team = team_for(*party_ctx, ctx.defender())) {
      const int vulnerable = field_debuff_value(*party_ctx, *defender_team,
                                                FieldDebuffKind::Vulnerable);
      if (vulnerable > 0) {
        scale_damage(damage, 100 + vulnerable);
      }
    }
  }

  const int shield_percent = status_value(ctx.reg(), ctx.defender(),
                                          CombatStatusKind::Shield);
  if (shield_percent > 0) {
    const int before = total_damage(damage);
    scale_damage(damage, std::max(0, 100 - shield_percent));
    const int reduced = std::max(0, before - total_damage(damage));
    if (reduced > 0) {
      ctx.log().append_markup(fmt::format(
          "[ability](Shield) absorbs [xp]({}) damage from {}.", reduced,
          ctx.log().name_tag_for(entt::handle{ctx.reg(), ctx.defender()})));
    }
  }
}

int CombatStatusSystem::heal(fl::context::PartyCtx &party_ctx,
                             entt::entity source, entt::entity target,
                             int amount, std::string_view label) {
  auto &reg = party_ctx.reg();
  if (!reg.valid(source) || !reg.valid(target) || amount <= 0) {
    return 0;
  }

  auto *stats = reg.try_get<Stats>(target);
  if (stats == nullptr || !stats->is_alive()) {
    return 0;
  }

  const int before = stats->hp_;
  stats->hp_ = std::min(stats->max_hp_, stats->hp_ + amount);
  const int healed = stats->hp_ - before;
  if (healed > 0) {
    party_ctx.log().append_markup(fmt::format(
        "{} restored {} for [xp]({}) HP with [ability]({}).",
        name_for(party_ctx, source), name_for(party_ctx, target), healed,
        label));
  }
  return healed;
}

int CombatStatusSystem::drain(fl::context::PartyCtx &party_ctx,
                              entt::entity source, entt::entity target,
                              fl::primitives::Damage damage, int heal_percent,
                              std::string_view label) {
  if (!party_ctx.reg().valid(source) || !party_ctx.reg().valid(target)) {
    return 0;
  }

  auto attack_ctx = fl::context::AttackCtx::make_attack(party_ctx, source, target);
  attack_ctx.damage() = damage;
  const int dealt = TakeDamage::commit(attack_ctx);
  if (dealt <= 0) {
    return 0;
  }

  return heal(party_ctx, source, source, dealt * std::max(0, heal_percent) / 100,
              label);
}

int CombatStatusSystem::cleanse(fl::context::PartyCtx &party_ctx,
                                entt::entity source, entt::entity target) {
  auto &reg = party_ctx.reg();
  if (!reg.valid(source) || !reg.valid(target)) {
    return 0;
  }

  int removed = 0;
  if (auto *statuses = reg.try_get<CombatStatuses>(target)) {
    std::vector<int> to_remove;
    for (const auto &effect : statuses->effects) {
      if (effect.negative && effect.removable) {
        to_remove.push_back(effect.id);
      }
    }
    for (const int id : to_remove) {
      clear_status_by_id(party_ctx, target, id);
      ++removed;
    }
  }

  if (reg.any_of<fl::ecs::components::Poison>(target)) {
    PoisonSystem::clear(party_ctx, target);
    ++removed;
  }
  if (reg.any_of<fl::ecs::components::Freeze>(target)) {
    FreezeSystem::clear(party_ctx, target);
    ++removed;
  }
  if (reg.any_of<fl::ecs::components::DireBleed>(target)) {
    DireBleedSystem::clear(party_ctx, target);
    ++removed;
  }

  party_ctx.log().append_markup(fmt::format(
      "{} used [ability](Cleanse) on {}; [xp]({}) effects removed.",
      name_for(party_ctx, source), name_for(party_ctx, target), removed));
  return removed;
}

void CombatStatusSystem::schedule_burn_tick(fl::context::PartyCtx &party_ctx,
                                            Scheduler &scheduler,
                                            entt::entity target,
                                            int status_id) {
  auto *burn = find_status_by_id(party_ctx.reg(), target, status_id);
  if (burn == nullptr) {
    return;
  }

  StatusEffectLifetime lifetime{party_ctx, scheduler, burn->effect};
  lifetime.schedule_in_beats(
      kBurnTickBeats, "burn: tick", [&party_ctx, &scheduler, target, status_id] {
        auto &reg = party_ctx.reg();
        auto *burn = find_status_by_id(reg, target, status_id);
        if (burn == nullptr || !reg.valid(target) || !reg.valid(burn->source) ||
            burn->ticks_remaining <= 0) {
          CombatStatusSystem::clear_status_by_id(party_ctx, target, status_id);
          return;
        }

        auto *target_stats = reg.try_get<Stats>(target);
        auto *source_stats = reg.try_get<Stats>(burn->source);
        if (target_stats == nullptr || source_stats == nullptr ||
            !target_stats->is_alive() || !source_stats->is_alive()) {
          CombatStatusSystem::clear_status_by_id(party_ctx, target, status_id);
          return;
        }

        auto attack_ctx = fl::context::AttackCtx::make_attack(party_ctx,
                                                              burn->source, target);
        attack_ctx.damage().fire = burn->tick_damage;
        party_ctx.log().append_markup(fmt::format(
            "[ability](Burn) scorches {} for [error]({}) damage.",
            name_for(party_ctx, target), burn->tick_damage));
        TakeDamage::commit(attack_ctx);

        if (auto *remaining = find_status_by_id(reg, target, status_id)) {
          --remaining->ticks_remaining;
          if (remaining->ticks_remaining > 0) {
            schedule_burn_tick(party_ctx, scheduler, target, status_id);
            return;
          }
        }

        CombatStatusSystem::clear_status_by_id(party_ctx, target, status_id);
      });
}

void CombatStatusSystem::clear_field_debuff_by_id(
    fl::context::PartyCtx &party_ctx, int field_id) {
  auto &reg = party_ctx.reg();
  auto *field = reg.try_get<FieldDebuffs>(party_ctx.self());
  if (field == nullptr) {
    return;
  }

  auto it = std::find_if(field->effects.begin(), field->effects.end(),
                         [field_id](const auto &effect) {
                           return effect.id == field_id;
                         });
  if (it == field->effects.end()) {
    return;
  }

  const auto effect_id = it->effect_id;
  const auto name = it->name;
  if (effect_id != entt::null) {
    auto &scheduler = party_ctx.party_data().encounter_data().atb_engine().scheduler();
    scheduler.clear_smelly_callbacks_for(effect_id);
    if (reg.valid(effect_id)) {
      reg.destroy(effect_id);
    }
  }

  field->effects.erase(it);
  party_ctx.log().append_markup(
      fmt::format("[ability]({}) fades from the field.", name));

  if (field->effects.empty()) {
    reg.remove<FieldDebuffs>(party_ctx.self());
  }
}

void CombatStatusSystem::apply_field_debuff(fl::context::PartyCtx &party_ctx,
                                            const FieldDebuffRequest &request) {
  auto &reg = party_ctx.reg();
  auto &field = reg.get_or_emplace<FieldDebuffs>(party_ctx.self());
  auto it = std::find_if(field.effects.begin(), field.effects.end(),
                         [&request](const auto &effect) {
                           return effect.team == request.team &&
                                  effect.kind == request.kind;
                         });
  if (it != field.effects.end()) {
    if (it->effect_id != entt::null) {
      auto &scheduler = party_ctx.party_data().encounter_data().atb_engine().scheduler();
      scheduler.clear_smelly_callbacks_for(it->effect_id);
      if (reg.valid(it->effect_id)) {
        reg.destroy(it->effect_id);
      }
    }
    field.effects.erase(it);
  }

  field.effects.push_back(fl::ecs::components::FieldDebuffEffect{
      .id = field.next_id++,
      .effect_id = entt::null,
      .team = request.team,
      .kind = request.kind,
      .name = std::string{request.name},
      .source = request.source,
      .value = request.value,
      .duration_seconds = request.duration_seconds,
      .removable = request.removable,
  });

  party_ctx.log().append_markup(fmt::format("[ability]({}) settles over the field.",
                                            request.name));
}

void CombatStatusSystem::apply_field_debuff(fl::context::PartyCtx &party_ctx,
                                            Scheduler &scheduler,
                                            const FieldDebuffRequest &request) {
  auto &reg = party_ctx.reg();
  auto &field = reg.get_or_emplace<FieldDebuffs>(party_ctx.self());
  auto it = std::find_if(field.effects.begin(), field.effects.end(),
                         [&request](const auto &effect) {
                           return effect.team == request.team &&
                                  effect.kind == request.kind;
                         });
  if (it != field.effects.end()) {
    if (it->effect_id != entt::null) {
      scheduler.clear_smelly_callbacks_for(it->effect_id);
      if (reg.valid(it->effect_id)) {
        reg.destroy(it->effect_id);
      }
    }
    field.effects.erase(it);
  }

  const int field_id = field.next_id++;
  const auto effect_id = request.duration_seconds > 0 ? reg.create() : entt::null;
  field.effects.push_back(fl::ecs::components::FieldDebuffEffect{
      .id = field_id,
      .effect_id = effect_id,
      .team = request.team,
      .kind = request.kind,
      .name = std::string{request.name},
      .source = request.source,
      .value = request.value,
      .duration_seconds = request.duration_seconds,
      .removable = request.removable,
  });

  party_ctx.log().append_markup(fmt::format("[ability]({}) settles over the field.",
                                            request.name));

  if (request.duration_seconds <= 0) {
    return;
  }

  const int clear_after_beats = fl::primitives::WorldClock::beats_from_seconds(
      std::max(1, request.duration_seconds));
  scheduler.schedule_smelly_in_beats_for(
      clear_after_beats, effect_id, "field debuff: expire",
      [&party_ctx, field_id] {
        CombatStatusSystem::clear_field_debuff_by_id(party_ctx, field_id);
      });
}

bool CombatStatusSystem::has_field_debuff(fl::context::PartyCtx &party_ctx,
                                          FieldTeam team,
                                          FieldDebuffKind kind) {
  return field_debuff_value(party_ctx, team, kind) > 0;
}

int CombatStatusSystem::field_debuff_value(fl::context::PartyCtx &party_ctx,
                                           FieldTeam team,
                                           FieldDebuffKind kind) {
  const auto *field = party_ctx.reg().try_get<FieldDebuffs>(party_ctx.self());
  if (field == nullptr) {
    return 0;
  }

  int value = 0;
  for (const auto &effect : field->effects) {
    if (effect.team == team && effect.kind == kind) {
      value = std::max(value, effect.value);
    }
  }
  return value;
}

int CombatStatusSystem::turn_tempo_modifier_percent(entt::registry &reg,
                                                    entt::entity actor) {
  return status_value(reg, actor, CombatStatusKind::Haste) -
         status_value(reg, actor, CombatStatusKind::Slow);
}

} // namespace fl::ecs::systems
