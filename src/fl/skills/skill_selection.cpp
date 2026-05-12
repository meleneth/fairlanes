#include "fl/skills/skill_selection.hpp"

#include <type_traits>
#include <vector>

#include "fl/ecs/components/skill_slots.hpp"
#include "fl/primitives/random_hub.hpp"

namespace fl::skills {

SkillId choose_skill(entt::registry &reg, fl::primitives::RandomHub &rng,
                     entt::entity actor) {
  const auto *slots = reg.try_get<fl::ecs::components::SkillSlots>(actor);
  if (slots == nullptr) {
    return SkillId::Thump;
  }

  std::vector<SkillId> known_combat_skills;
  for (const auto skill : kRandomCombatSkills) {
    if (slots->knows(skill)) {
      known_combat_skills.push_back(skill);
    }
  }

  if (known_combat_skills.empty()) {
    return SkillId::Observe;
  }

  auto rs =
      rng.stream("encounter/choose-skill",
                 static_cast<std::underlying_type_t<entt::entity>>(actor));
  return known_combat_skills[static_cast<std::size_t>(
      rs.random_index(static_cast<int>(known_combat_skills.size())))];
}

} // namespace fl::skills
