#pragma once

#include "fl/monsters/monster_kind.hpp"
#include "fl/skills/skill.hpp"

namespace fl::monster {

fl::skills::SkillId known_skill_for(MonsterKind kind) noexcept;

} // namespace fl::monster
