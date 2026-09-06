#include "fl/monsters/decision_rule.hpp"

#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/combat_status_system.hpp"
#include "fl/skills/skill_selection.hpp"
#include <algorithm>
#include <array>
#include <cstdint>

namespace fl::monster {

bool has_rule_status(entt::registry &reg, entt::entity entity,
                     RuleStatus status) {
  using fl::ecs::components::CombatStatusKind;
  using fl::ecs::systems::CombatStatusSystem;
  if (!reg.valid(entity))
    return false;
  switch (status) {
  case RuleStatus::None:
    return false;
  case RuleStatus::Poison:
    return reg.all_of<fl::ecs::components::Poison>(entity);
  case RuleStatus::DireBleed:
    return reg.all_of<fl::ecs::components::DireBleed>(entity);
  case RuleStatus::Freeze:
    return reg.all_of<fl::ecs::components::Freeze>(entity);
  case RuleStatus::Shield:
    return CombatStatusSystem::has_status(reg, entity,
                                          CombatStatusKind::Shield);
  case RuleStatus::Haste:
    return CombatStatusSystem::has_status(reg, entity, CombatStatusKind::Haste);
  case RuleStatus::Burn:
    return CombatStatusSystem::has_status(reg, entity, CombatStatusKind::Burn);
  case RuleStatus::Blind:
    return CombatStatusSystem::has_status(reg, entity, CombatStatusKind::Blind);
  case RuleStatus::Silence:
    return CombatStatusSystem::has_status(reg, entity,
                                          CombatStatusKind::Silence);
  case RuleStatus::Slow:
    return CombatStatusSystem::has_status(reg, entity, CombatStatusKind::Slow);
  case RuleStatus::Stun:
    return CombatStatusSystem::has_status(reg, entity, CombatStatusKind::Stun);
  }
  return false;
}

namespace {
bool alive(entt::registry &reg, entt::entity entity) {
  const auto *stats = reg.try_get<fl::ecs::components::Stats>(entity);
  return stats && stats->hp_ > 0;
}

bool matches(entt::registry &reg, entt::entity actor, entt::entity target,
             const RuleCondition &condition) {
  const auto subject = condition.subject == RuleSubject::Actor ? actor : target;
  switch (condition.predicate) {
  case RulePredicate::HasStatus:
    return has_rule_status(reg, subject, condition.status);
  case RulePredicate::MissingStatus:
    return !has_rule_status(reg, subject, condition.status);
  case RulePredicate::HpBelow:
  case RulePredicate::HpAbove: {
    const auto *stats = reg.try_get<fl::ecs::components::Stats>(subject);
    if (!stats || stats->max_hp_ <= 0)
      return false;
    const auto hp = std::int64_t{stats->hp_} * 100;
    const auto threshold = std::int64_t{stats->max_hp_} * condition.percent;
    return condition.predicate == RulePredicate::HpBelow ? hp < threshold
                                                         : hp > threshold;
  }
  }
  return false;
}
} // namespace

std::optional<SkillDecision>
evaluate_rules(entt::registry &reg, entt::entity actor,
               std::span<const entt::entity> allies,
               std::span<const entt::entity> enemies,
               std::span<const DecisionRule> rules,
               const std::function<int()> &roll_percent) {
  if (!alive(reg, actor))
    return std::nullopt;
  const auto *slots = reg.try_get<fl::ecs::components::SkillSlots>(actor);
  if (!slots)
    return std::nullopt;
  const std::array self{actor};
  for (const auto &rule : rules) {
    if (rule.chance_percent <= 0 || rule.chance_percent > 100)
      continue;
    std::optional<fl::skills::SkillKey> skill;
    for (auto equipped : slots->slots) {
      if (equipped && equipped->base == rule.skill &&
          fl::ecs::systems::CombatStatusSystem::can_use_skill(reg, actor,
                                                              *equipped) &&
          (!skill || skill->rank < equipped->rank)) {
        skill = equipped;
      }
    }
    if (!skill)
      continue;
    const auto candidates = rule.target == RuleTarget::Self
                                ? std::span<const entt::entity>{self}
                            : rule.target == RuleTarget::Ally ? allies
                                                              : enemies;
    const auto target =
        std::ranges::find_if(candidates, [&](entt::entity candidate) {
          return alive(reg, candidate) &&
                 fl::skills::target_meets_skill_requirements(reg, candidate,
                                                             *skill) &&
                 std::ranges::all_of(
                     rule.conditions, [&](const auto &condition) {
                       return matches(reg, actor, candidate, condition);
                     });
        });
    if (target == candidates.end())
      continue;
    if (rule.chance_percent < 100) {
      const int roll = roll_percent();
      if (roll < 1 || roll > rule.chance_percent)
        continue;
    }
    return SkillDecision{*skill, *target};
  }
  return std::nullopt;
}

} // namespace fl::monster
