#include "closet.hpp"

namespace fl::ecs::components {

Closet::Closet()
    : chest(entt::null), helm(entt::null), pants(entt::null), belt(entt::null),
      boots(entt::null), gloves(entt::null), sleeves(entt::null),
      cape(entt::null), necklace(entt::null), ring_1(entt::null),
      ring_2(entt::null), mainhand(entt::null), offhand(entt::null),
      knife(entt::null),
      skill_slots{fl::skills::SkillKey{fl::skills::SkillId::Observe},
                  std::nullopt, std::nullopt, std::nullopt, std::nullopt} {}

bool Closet::empty() const {
  return chest == entt::null && helm == entt::null && pants == entt::null &&
         belt == entt::null && boots == entt::null && gloves == entt::null &&
         sleeves == entt::null && cape == entt::null &&
         necklace == entt::null && ring_1 == entt::null &&
         ring_2 == entt::null && mainhand == entt::null &&
         offhand == entt::null && knife == entt::null;
}

bool Closet::has_equipped_skill(fl::skills::SkillKey skill) const noexcept {
  for (const auto equipped : skill_slots) {
    if (equipped == skill) {
      return true;
    }
  }
  return false;
}

bool Closet::has_open_skill_slot() const noexcept {
  for (const auto equipped : skill_slots) {
    if (!equipped.has_value()) {
      return true;
    }
  }
  return false;
}

bool Closet::equip_skill(fl::skills::SkillKey skill) noexcept {
  if (has_equipped_skill(skill)) {
    return false;
  }

  for (auto &equipped : skill_slots) {
    if (!equipped.has_value()) {
      equipped = skill;
      return true;
    }
  }

  return false;
}

bool Closet::unequip_skill(fl::skills::SkillKey skill) noexcept {
  for (auto &equipped : skill_slots) {
    if (equipped == skill) {
      equipped.reset();
      return true;
    }
  }

  return false;
}

} // namespace fl::ecs::components
