#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "fl/skills/skill.hpp"
#include "fl/widgets/effects/decal.hpp"

namespace fl::skills {

struct SkillDefinition {
  SkillId kind;
  std::string_view display_name;
  std::span<const SkillTag> tags;
  int learn_chance_percent;
  int flee_success_percent;
  SkillExecutionKind execution;
  std::optional<fl::widgets::effects::DecalAnimationKind> decal_animation;
};

std::span<const SkillDefinition *const> all_definitions() noexcept;

} // namespace fl::skills
