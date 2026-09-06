#include "fl/primitives/encounter_data.hpp"

#include <fmt/format.h>

#include <optional>
#include <ranges>

#include "fl/assert.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/monster_identity.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/combat_status_system.hpp"
#include "fl/ecs/systems/freeze_system.hpp"
#include "fl/ecs/systems/poison_system.hpp"
#include "fl/generated/monster_content.hpp"
#include "fl/primitives/member_data.hpp"
#include "fl/primitives/random_hub.hpp"
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

entt::entity random_target(fl::targeting::TargetRange candidates,
                           fl::primitives::RandomHub &rng) {
  auto random = rng.stream("encounter/team");
  auto selected = entt::entity{entt::null};
  int seen = 0;
  for (const auto candidate : candidates) {
    if (random.random_index(++seen) == 0)
      selected = candidate;
  }
  return selected;
}

} // namespace

void EncounterData::innervate_event_system() {
  ZoneScopedN("EncounterData::innervate_event_system");
  wire_.atb_active_ = atb_out().subscribe<seerin::BecameActive>(
      [this](const seerin::BecameActive &ev) {
        ZoneScopedN("EncounterData::BecameActive");
        const entt::entity attacker = ev.id;
        if (fl::ecs::systems::CombatStatusSystem::consume_stun_turn(*party_ctx_,
                                                                    attacker)) {
          atb_in().emit(seerin::AtbInEvent{seerin::FinishedTurn{attacker}});
          return;
        }

        const auto decision = choose_action(attacker);
        const entt::entity target = decision ? decision->target : entt::null;

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
        sequencer.schedule(attacker, target, decision->skill);
        TracyPlot("Encounter.PendingEvents",
                  static_cast<double>(rt_.atb_.scheduler().pending()));
      });
}

fl::skills::SkillKey EncounterData::choose_skill(entt::entity attacker) {
  ZoneScopedN("EncounterData::choose_skill");
  return fl::skills::choose_skill(party_ctx_->reg(), party_ctx_->rng(),
                                  attacker);
}

std::optional<fl::monster::SkillDecision>
EncounterData::choose_action(entt::entity actor) {
  if (const auto *monster =
          party_ctx_->reg().try_get<fl::ecs::components::MonsterIdentity>(
              actor)) {
    const auto rules =
        fl::monster::generated_content::decision_rules(monster->kind);
    if (!rules.empty()) {
      auto allies = possible_targets().FriendlyPossibleTargets(actor);
      auto enemies = possible_targets().EnemyPossibleTargets(actor);
      auto random = party_ctx_->rng().stream("encounter/monster-rule",
                                             entt::to_integral(actor));
      return fl::monster::evaluate_rules(
          party_ctx_->reg(), actor, allies, enemies, rules,
          [&random] { return random.uniform_int<int>(1, 100); });
    }
  }
  const auto skill = choose_skill(actor);
  return fl::monster::SkillDecision{skill, target_for_skill(actor, skill)};
}

entt::entity
EncounterData::target_random_alive_opposition(entt::entity actor) const {
  return random_target(possible_targets().EnemyPossibleTargets(actor),
                       party_ctx_->rng());
}

entt::entity EncounterData::target_for_skill(entt::entity actor,
                                             fl::skills::SkillKey skill) const {
  ZoneScopedN("EncounterData::target_for_skill");
  auto candidates = possible_targets();
  if (!party_ctx_->reg().valid(actor) ||
      (!attackers().contains(actor) && !defenders().contains(actor)))
    return entt::null;
  if (fl::skills::has_tag(skill, fl::skills::SkillTag::Self)) {
    for (auto candidate : candidates.FriendlyPossibleTargets(actor))
      if (candidate == actor)
        return actor;
    return entt::null;
  }
  if (fl::skills::definition(skill).consumes_status) {
    for (auto candidate : candidates.EnemyPossibleTargets(actor)) {
      if (fl::skills::target_meets_skill_requirements(party_ctx_->reg(),
                                                      candidate, skill))
        return candidate;
    }
    return entt::null;
  }
  if (fl::skills::has_tag(skill, fl::skills::SkillTag::Healing)) {
    auto selected = entt::entity{entt::null};
    int lowest_hp = 0;
    for (auto candidate : candidates.FriendlyPossibleTargets(actor)) {
      const auto hp =
          party_ctx_->reg().get<fl::ecs::components::Stats>(candidate).hp_;
      if (selected == entt::null || hp < lowest_hp) {
        selected = candidate;
        lowest_hp = hp;
      }
    }
    return selected;
  }
  const bool cleanse =
      fl::skills::has_tag(skill, fl::skills::SkillTag::Cleanse);
  if (cleanse) {
    auto eligible = candidates.FriendlyPossibleTargets(actor) |
                    std::views::filter([this](entt::entity target) {
                      return has_cleansable_debuff(party_ctx_->reg(), target);
                    });
    for (auto candidate : eligible)
      return candidate;
  }
  const bool friendly =
      cleanse || fl::skills::has_tag(skill, fl::skills::SkillTag::Heal) ||
      fl::skills::has_tag(skill, fl::skills::SkillTag::Ally) ||
      fl::skills::has_tag(skill, fl::skills::SkillTag::AllAllies) ||
      fl::skills::has_tag(skill, fl::skills::SkillTag::Buff);
  return random_target(friendly ? candidates.FriendlyPossibleTargets(actor)
                                : candidates.EnemyPossibleTargets(actor),
                       party_ctx_->rng());
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
    return 100 +
           fl::ecs::systems::CombatStatusSystem::turn_tempo_modifier_percent(
               party_ctx_->reg(), entity);
  });

  wire_.party_beat_ = fl::events::ScopedPartyListener{
      party_ctx_->bus(), std::in_place_type<fl::events::PartyTick>,
      [this](const fl::events::PartyTick &) { atb_in().emit(seerin::Beat{}); }};
}

} // namespace fl::primitives
