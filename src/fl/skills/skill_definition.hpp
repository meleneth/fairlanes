#pragma once

#include <optional>
#include <span>
#include <string_view>
#include <string>

#include "fl/ecs/components/combat_status_kind.hpp"
#include "fl/skills/skill.hpp"
#include "fl/skills/skill_scaling.hpp"
#include "fl/widgets/effects/decal_animation_kind.hpp"

namespace fl::skills {

struct SkillResourcePolicy {
  fl::resources::CombatResource resource{fl::resources::CombatResource::Adrenaline};
  int cost{0};
  int generate_on_hit{0};
  int restore_on_resolution{0};
  fl::resources::CombatResource target_resource{fl::resources::CombatResource::Adrenaline};
  int target_delta{0};
};

struct SkillStudyPolicy {
  bool required{false};
  int duration_seconds{0};
  int required_proficiency{0};
};

struct SkillDefinition {
  SkillKey key;
  std::string_view display_name;
  std::span<const SkillTag> tags;
  int learn_chance_percent;
  int flee_success_percent;
  SkillExecutionKind execution;
  std::optional<fl::widgets::effects::DecalAnimationKind> decal_animation;
  std::string_view description;
  std::optional<fl::ecs::components::CombatStatusKind> consumes_status{};
  int effect_damage{0};
  SkillScalingRule scaling{};
  SkillResourcePolicy resources{};
  SkillStudyPolicy study{};
};

std::span<const SkillDefinition* const> all_definitions() noexcept;
std::string describe_resource_policy(const SkillResourcePolicy& policy);

} // namespace fl::skills
