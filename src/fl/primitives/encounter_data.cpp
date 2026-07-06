#include "fl/primitives/encounter_data.hpp"

#include <fmt/format.h>

#include <optional>
#include <ranges>

#include "fl/assert.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/combat_status_system.hpp"
#include "fl/ecs/systems/freeze_system.hpp"
#include "fl/ecs/systems/poison_system.hpp"
#include "fl/primitives/member_data.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/skills/skill_selection.hpp"
#include "fl/skills/skill_sequence.hpp"
#include "fl/tracy_shim.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "sr/atb_events.hpp"

namespace fl::primitives {
namespace {

bool has_cleansable_debuff(entt::registry &reg, entt::entity target) {
  using fl::ecs::components::CombatStatuses;
  using fl::ecs::components::DireBleed;
  using fl::ecs::components::Freeze;
  using fl::ecs::components::Poison;

  if (auto *statuses = reg.try_get<CombatStatuses>(target)) {
    const auto has_removable_negative = [](const auto &effect) {
      return effect.negative && effect.removable;
    };
    if (std::ranges::any_of(statuses->effects, has_removable_negative)) {
      return true;
    }
  }

  return reg.any_of<Poison, Freeze, DireBleed>(target);
}

std::optional<entt::entity> cleansable_member(const Team &team,
                                              fl::context::PartyCtx &ctx) {
  std::optional<entt::entity> selected;
  team.for_each_alive_member(ctx, [&](entt::entity candidate) {
    if (!selected.has_value() && has_cleansable_debuff(ctx.reg(), candidate)) {
      selected = candidate;
    }
  });
  return selected;
}

} // namespace

void EncounterData::innervate_event_system() {
  ZoneScopedN("EncounterData::innervate_event_system");
  wire_.atb_active_ = atb_out().subscribe<seerin::BecameActive>(
      [this](const seerin::BecameActive &ev) {
        ZoneScopedN("EncounterData::BecameActive");
        const entt::entity attacker = ev.id;
        if (fl::ecs::systems::CombatStatusSystem::consume_stun_turn(
                *party_ctx_, attacker)) {
          atb_in().emit(seerin::AtbInEvent{seerin::FinishedTurn{attacker}});
          return;
        }

        const auto skill = choose_skill(attacker);
        const entt::entity target = target_for_skill(attacker, skill);

        if (target == entt::null) {
          party_ctx_->log().append_markup(fmt::format(
              "{} did nothing.", party_ctx_->reg()
                                     .get<fl::ecs::components::Stats>(attacker)
                                     .name_));

          atb_in().emit(seerin::AtbInEvent{seerin::FinishedTurn{attacker}});
          return;
        }

        fl::skills::SkillSequencer sequencer{
            *party_ctx_, rt_.atb_.scheduler(), [this](entt::entity entity) {
              atb_in().emit(seerin::AtbInEvent{seerin::FinishedTurn{entity}});
            }};
        sequencer.schedule(attacker, target, skill);
        TracyPlot("Encounter.PendingEvents",
                  static_cast<double>(rt_.atb_.scheduler().pending()));
      });
}

fl::skills::SkillKey EncounterData::choose_skill(entt::entity attacker) {
  ZoneScopedN("EncounterData::choose_skill");
  return fl::skills::choose_skill(party_ctx_->reg(), party_ctx_->rng(),
                                  attacker);
}

entt::entity EncounterData::target_for_skill(entt::entity attacker,
                                             fl::skills::SkillKey skill) const {
  ZoneScopedN("EncounterData::target_for_skill");
  if (fl::skills::has_tag(skill, fl::skills::SkillTag::Healing)) {
    if (topo_.attackers_.contains(attacker)) {
      return topo_.attackers_.least_health_member(*party_ctx_)
          .value_or(entt::null);
    }

    if (topo_.defenders_.contains(attacker)) {
      return topo_.defenders_.least_health_member(*party_ctx_)
          .value_or(entt::null);
    }

    return entt::null;
  }

  const bool is_cleanse =
      fl::skills::has_tag(skill, fl::skills::SkillTag::Cleanse);
  if (is_cleanse) {
    if (topo_.attackers_.contains(attacker)) {
      if (auto target = cleansable_member(topo_.attackers_, *party_ctx_)) {
        return *target;
      }
    }

    if (topo_.defenders_.contains(attacker)) {
      if (auto target = cleansable_member(topo_.defenders_, *party_ctx_)) {
        return *target;
      }
    }
  }

  const bool targets_ally =
      fl::skills::has_tag(skill, fl::skills::SkillTag::Heal) ||
      fl::skills::has_tag(skill, fl::skills::SkillTag::Ally) ||
      fl::skills::has_tag(skill, fl::skills::SkillTag::AllAllies) ||
      is_cleanse || fl::skills::has_tag(skill, fl::skills::SkillTag::Buff);

  if (targets_ally) {
    if (topo_.attackers_.contains(attacker)) {
      return topo_.attackers_.random_alive_member(*party_ctx_)
          .value_or(entt::null);
    }

    if (topo_.defenders_.contains(attacker)) {
      return topo_.defenders_.random_alive_member(*party_ctx_)
          .value_or(entt::null);
    }
  }

  return target_random_alive_opposition(attacker);
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

void EncounterData::clear_active_turn_for(entt::entity id) {
  ZoneScopedN("EncounterData::clear_active_turn_for");
  rt_.atb_.clear_active_turn_for(id);
}

fl::events::CombatantBus &
EncounterData::add_enemy_combatant_bus(entt::entity enemy) {
  if (auto *bus = enemy_combatant_bus(enemy)) {
    bind_combatant_bus(enemy, *bus);
    return *bus;
  }

  rt_.enemy_combatant_buses_.push_back(
      EnemyCombatantBus{.enemy = enemy, .bus = {}});
  auto &bus = rt_.enemy_combatant_buses_.back().bus;
  bind_combatant_bus(enemy, bus);
  return bus;
}

fl::events::CombatantBus &
EncounterData::add_party_combatant_bus(entt::entity member) {
  auto &bus = combatant_bus(member);
  bind_combatant_bus(member, bus);
  return bus;
}

fl::events::CombatantBus &EncounterData::combatant_bus(entt::entity combatant) {
  if (auto *bus = enemy_combatant_bus(combatant)) {
    return *bus;
  }

  auto *member =
      party_ctx_->reg().try_get<fl::ecs::components::PartyMember>(combatant);
  if (member == nullptr) {
    fl::fail("combatant bus requested for an entity not enrolled as an enemy "
             "or party member");
  }
  return member->member_data().combatant_bus();
}

const fl::events::CombatantBus &
EncounterData::combatant_bus(entt::entity combatant) const {
  if (auto *bus = enemy_combatant_bus(combatant)) {
    return *bus;
  }

  auto *member =
      party_ctx_->reg().try_get<fl::ecs::components::PartyMember>(combatant);
  if (member == nullptr) {
    fl::fail("combatant bus requested for an entity not enrolled as an enemy "
             "or party member");
  }
  return member->member_data().combatant_bus();
}

fl::events::CombatantBus *
EncounterData::enemy_combatant_bus(entt::entity enemy) {
  auto it = std::find_if(
      rt_.enemy_combatant_buses_.begin(), rt_.enemy_combatant_buses_.end(),
      [enemy](const EnemyCombatantBus &slot) { return slot.enemy == enemy; });
  if (it == rt_.enemy_combatant_buses_.end()) {
    return nullptr;
  }
  return &it->bus;
}

const fl::events::CombatantBus *
EncounterData::enemy_combatant_bus(entt::entity enemy) const {
  auto it = std::find_if(
      rt_.enemy_combatant_buses_.begin(), rt_.enemy_combatant_buses_.end(),
      [enemy](const EnemyCombatantBus &slot) { return slot.enemy == enemy; });
  if (it == rt_.enemy_combatant_buses_.end()) {
    return nullptr;
  }
  return &it->bus;
}

void EncounterData::bind_combatant_bus(
    entt::entity combatant, fl::events::CombatantBus &combatant_bus) {
  if (std::find(wire_.wired_combatants_.begin(), wire_.wired_combatants_.end(),
                combatant) != wire_.wired_combatants_.end()) {
    return;
  }

  auto &wiring = wire_.combatant_wiring_.emplace_back();
  wiring.poison_apply_ = fl::ecs::systems::PoisonSystem::bind_apply_listener(
      *party_ctx_, combatant_bus, rt_.atb_.scheduler());

  wiring.freeze_apply_ = fl::ecs::systems::FreezeSystem::bind_apply_listener(
      *party_ctx_, combatant_bus, rt_.atb_.scheduler());

  wiring.freeze_started_ = fl::events::ScopedCombatantListener{
      combatant_bus, std::in_place_type<fl::events::FreezeStarted>,
      [this](const fl::events::FreezeStarted &ev) {
        atb_in().emit(seerin::AtbInEvent{seerin::Frozen{ev.target}});
      }};

  wiring.freeze_ended_ = fl::events::ScopedCombatantListener{
      combatant_bus, std::in_place_type<fl::events::FreezeEnded>,
      [this](const fl::events::FreezeEnded &ev) {
        atb_in().emit(seerin::AtbInEvent{seerin::Thawed{ev.target}});
      }};

  wire_.wired_combatants_.push_back(combatant);
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
  rt_.atb_.bind_registry(party_ctx_->reg());

  rt_.atb_.set_can_charge_fn([this](entt::entity entity) {
    auto *stats = party_ctx_->reg().try_get<fl::ecs::components::Stats>(entity);
    return stats && stats->is_alive();
  });

  rt_.atb_.set_charge_rate_percent_fn([this](entt::entity entity) {
    return 100 + fl::ecs::systems::CombatStatusSystem::turn_tempo_modifier_percent(
                     party_ctx_->reg(), entity);
  });

  wire_.party_beat_ = fl::events::ScopedPartyListener{
      party_ctx_->bus(), std::in_place_type<fl::events::PartyTick>,
      [this](const fl::events::PartyTick &) { atb_in().emit(seerin::Beat{}); }};
}

} // namespace fl::primitives
