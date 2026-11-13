#pragma once

#include <array>

#include "fl/resources/combat_resource.hpp"
#include "fl/skills/skill.hpp"

namespace fl::ecs::components {

struct ResourceCastSnapshot {
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
  std::array<int, fl::resources::index(fl::resources::CombatResource::Count)> current{};
  std::array<int, fl::resources::index(fl::resources::CombatResource::Count)> maximum{};
  bool resolution_applied{false};
};

} // namespace fl::ecs::components
