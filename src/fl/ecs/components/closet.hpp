#pragma once

#include <array>
#include <cstdlib>
#include <optional>

#include <entt/entt.hpp>

#include "fl/skills/skill.hpp"

namespace fl::ecs::components {

struct Closet {
  static constexpr int kSkillSlotCount = 5;

  entt::entity chest;
  entt::entity helm;
  entt::entity pants;
  entt::entity belt;
  entt::entity boots;
  entt::entity gloves;
  entt::entity sleeves;
  entt::entity cape;

  entt::entity necklace;
  entt::entity ring_1;
  entt::entity ring_2;

  entt::entity mainhand;
  entt::entity offhand;

  entt::entity knife;

  std::array<std::optional<fl::skills::SkillKey>, kSkillSlotCount> skill_slots;

  Closet();

  [[nodiscard]] bool
  has_equipped_skill(fl::skills::SkillKey skill) const noexcept;
  [[nodiscard]] bool has_open_skill_slot() const noexcept;
  bool equip_skill(fl::skills::SkillKey skill) noexcept;
  bool unequip_skill(fl::skills::SkillKey skill) noexcept;

  bool empty() const;
};

} // namespace fl::ecs::components
