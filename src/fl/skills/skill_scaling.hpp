#pragma once

#include <algorithm>

#include "fl/ecs/systems/effective_combat_stats.hpp"

namespace fl::skills {

enum class ScalingStat { Attack, Armor };
enum class ScalingDimension { FlatDamage, MitigationPercent };

struct SkillScalingRule {
  ScalingStat stat;
  ScalingDimension dimension;
  int numerator;
  int denominator;
  int maximum_bonus;
};

// Experimental first-slice relationships. Keeping the coefficients as data
// makes it clear that Attack is not a universal damage multiplier.
inline constexpr SkillScalingRule kThumpAttackScaling{ScalingStat::Attack,
                                                      ScalingDimension::FlatDamage, 1, 1, 0};
inline constexpr SkillScalingRule kBraceArmorScaling{ScalingStat::Armor,
                                                     ScalingDimension::MitigationPercent, 2, 1, 35};

inline int thump_attack_bonus(const fl::ecs::systems::EffectiveCombatStats& stats) noexcept {
  return stats.attack * kThumpAttackScaling.numerator / kThumpAttackScaling.denominator;
}

inline int brace_mitigation_percent(const fl::ecs::systems::EffectiveCombatStats& stats) noexcept {
  constexpr int base = 35;
  const int bonus = stats.armor * kBraceArmorScaling.numerator / kBraceArmorScaling.denominator;
  return base + std::clamp(bonus, 0, kBraceArmorScaling.maximum_bonus);
}

} // namespace fl::skills
