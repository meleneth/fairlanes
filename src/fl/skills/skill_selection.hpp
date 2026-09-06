#pragma once

#include <entt/entt.hpp>

#include "fl/skills/skill.hpp"

namespace fl::primitives {
class RandomHub;
}

namespace fl::skills {

SkillKey choose_skill(entt::registry &reg, fl::primitives::RandomHub &rng,
                      entt::entity actor);
bool target_meets_skill_requirements(entt::registry &reg, entt::entity target,
                                     SkillKey skill);

} // namespace fl::skills
