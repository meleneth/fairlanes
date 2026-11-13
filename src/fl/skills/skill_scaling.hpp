#pragma once

#include <string>
#include <string_view>
#include <optional>

#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>

#include "fl/primitives/combat_affinity.hpp"
#include "fl/resources/combat_resource.hpp"
#include "fl/skills/skill.hpp"

namespace fl::skills {

enum class ScalingStat { None, Attack, Armor };
enum class ScalingDimension { Neutral, Damage, Healing, Mitigation, Control, Accuracy, Timing };
enum class ResourceScalingSource { None, Current, CurrentPercent, Maximum };

struct SkillScalingRule {
  fl::primitives::CombatAffinity affinity{fl::primitives::CombatAffinity::Neutral};
  ScalingStat stat{ScalingStat::None};
  ScalingDimension dimension{ScalingDimension::Neutral};
  int base_effect{0};
  int numerator{0};
  int denominator{1};
  int maximum_bonus{0};
  int affinity_percent{0};
  fl::resources::CombatResource resource{fl::resources::CombatResource::Adrenaline};
  ResourceScalingSource resource_source{ResourceScalingSource::None};
  int resource_numerator{0};
  int resource_denominator{1};
  int resource_maximum_bonus{0};
};

struct SkillScalingResult {
  int base{0};
  int stat_bonus{0};
  int affinity_bonus{0};
  int resource_bonus{0};
  int total{0};
};

SkillScalingResult evaluate_scaling(entt::registry& reg, entt::entity actor,
                                    const SkillScalingRule& rule, int base,
                                    std::optional<SkillKey> skill = std::nullopt) noexcept;
std::string describe_scaling(const SkillScalingRule& rule);
std::string_view to_string(ScalingDimension dimension) noexcept;

} // namespace fl::skills
