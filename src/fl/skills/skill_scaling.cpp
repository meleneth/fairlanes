#include "fl/skills/skill_scaling.hpp"

#include <algorithm>
#include <fmt/format.h>

#include "fl/ecs/components/combat_affinity.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"

namespace fl::skills {
namespace {
bool affinity_matches(entt::registry& reg, entt::entity actor,
                      fl::primitives::CombatAffinity affinity) {
  if (affinity == fl::primitives::CombatAffinity::Neutral) return false;
  if (const auto* active = reg.try_get<fl::ecs::components::ActiveCombatAffinity>(actor))
    return active->value == affinity;
  if (const auto* set = reg.try_get<fl::ecs::components::CombatAffinities>(actor))
    return set->values.contains(affinity);
  return false;
}
} // namespace

SkillScalingResult evaluate_scaling(entt::registry& reg, entt::entity actor,
                                    const SkillScalingRule& rule, int base) noexcept {
  SkillScalingResult result{.base = base};
  if (!reg.valid(actor)) {
    result.total = base;
    return result;
  }
  const auto stats = fl::ecs::systems::effective_combat_stats(reg, actor);
  const int stat = rule.stat == ScalingStat::Attack  ? stats.attack
                   : rule.stat == ScalingStat::Armor ? stats.armor
                                                     : 0;
  if (rule.denominator > 0)
    result.stat_bonus = std::clamp(stat * rule.numerator / rule.denominator, 0, rule.maximum_bonus);
  const int subtotal = std::max(0, base + result.stat_bonus);
  if (affinity_matches(reg, actor, rule.affinity))
    result.affinity_bonus = subtotal * rule.affinity_percent / 100;
  result.total = std::max(0, subtotal + result.affinity_bonus);
  return result;
}

std::string describe_scaling(const SkillScalingRule& rule) {
  if (rule.dimension == ScalingDimension::Neutral)
    return fmt::format("= base {} · mechanically neutral (no numeric scaling)", rule.base_effect);
  const auto stat = rule.stat == ScalingStat::Attack  ? "Attack"
                    : rule.stat == ScalingStat::Armor ? "Armor"
                                                      : "none";
  return fmt::format("= base {} + {}×{}/{} (MAX {})  × {} affinity +{}%", rule.base_effect, stat,
                     rule.numerator, rule.denominator, rule.maximum_bonus,
                     fl::primitives::to_string(rule.affinity), rule.affinity_percent);
}

std::string_view to_string(ScalingDimension dimension) noexcept {
  switch (dimension) {
  case ScalingDimension::Neutral: return "neutral";
  case ScalingDimension::Damage: return "damage";
  case ScalingDimension::Healing: return "healing";
  case ScalingDimension::Mitigation: return "mitigation";
  case ScalingDimension::Control: return "control";
  case ScalingDimension::Accuracy: return "accuracy";
  case ScalingDimension::Timing: return "timing";
  }
  return "unknown";
}
} // namespace fl::skills
