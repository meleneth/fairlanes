#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "fl/ecs/components/combat_status.hpp"
#include "fl/skills/skill.hpp"
#include "fl/widgets/effects/decal.hpp"

namespace fl::skills {

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
};

std::span<const SkillDefinition *const> all_definitions() noexcept;

} // namespace fl::skills
