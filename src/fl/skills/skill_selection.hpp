#pragma once

#include <entt/entt.hpp>

#include "fl/skills/skill.hpp"

namespace fl::primitives {
class RandomHub;
}

namespace fl::skills {

SkillId choose_skill(entt::registry &reg, fl::primitives::RandomHub &rng,
                     entt::entity actor);

} // namespace fl::skills
