#include "fl/skills/skill_selection.hpp"

#include <array>
#include <optional>
#include <type_traits>
#include <vector>

#include "fl/ecs/components/closet.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/primitives/random_hub.hpp"

namespace fl::skills {
namespace {

using EquippedSkills = std::array<std::optional<SkillKey>,
                                  fl::ecs::components::Closet::kSkillSlotCount>;

const EquippedSkills *equipped_skills_for(entt::registry &reg,
                                          entt::entity actor) {
  if (const auto *member =
          reg.try_get<fl::ecs::components::PartyMember>(actor)) {
    return &member->closet().skill_slots;
  }

  if (const auto *slots = reg.try_get<fl::ecs::components::SkillSlots>(actor)) {
    return &slots->slots;
  }

  return nullptr;
}

} // namespace

SkillKey choose_skill(entt::registry &reg, fl::primitives::RandomHub &rng,
                      entt::entity actor) {
  const auto *equipped_skills = equipped_skills_for(reg, actor);
  if (equipped_skills == nullptr) {
    return SkillKey{SkillId::Thump};
  }

  std::vector<SkillKey> known_combat_skills;
  for (const auto skill : kRandomCombatSkills) {
    for (const auto equipped : *equipped_skills) {
      if (equipped == skill) {
        known_combat_skills.push_back(skill);
        break;
      }
    }
  }

  if (known_combat_skills.empty()) {
    return SkillKey{SkillId::Observe};
  }

  auto rs =
      rng.stream("encounter/choose-skill",
                 static_cast<std::underlying_type_t<entt::entity>>(actor));
  return known_combat_skills[static_cast<std::size_t>(
      rs.random_index(static_cast<int>(known_combat_skills.size())))];
}

} // namespace fl::skills
