#pragma once

#include <optional>

#include "fl/skills/skill.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/widgets/effects/decal.hpp"

namespace fl::skills {

[[nodiscard]] inline std::optional<fl::widgets::effects::DecalAnimationKind>
decal_animation_for(SkillKey skill) noexcept {
  return definition(skill).decal_animation;
}

} // namespace fl::skills
