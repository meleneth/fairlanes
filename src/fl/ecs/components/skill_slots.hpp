#pragma once

#include <array>
#include <optional>

#include "fl/skills/skill.hpp"

namespace fl::ecs::components {

struct SkillSlots {
  static constexpr int kSlotCount = 5;

  std::array<std::optional<fl::skills::SkillId>, kSlotCount> slots{
      fl::skills::SkillId::Observe, std::nullopt, std::nullopt, std::nullopt,
      std::nullopt};

  static SkillSlots with_known(fl::skills::SkillId skill) noexcept {
    SkillSlots known;
    known.learn(skill);
    return known;
  }

  [[nodiscard]] bool knows(fl::skills::SkillId skill) const noexcept {
    for (const auto known : slots) {
      if (known == skill) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] bool has_open_slot() const noexcept {
    for (const auto known : slots) {
      if (!known.has_value()) {
        return true;
      }
    }
    return false;
  }

  bool learn(fl::skills::SkillId skill) noexcept {
    if (knows(skill)) {
      return false;
    }

    for (auto &known : slots) {
      if (!known.has_value()) {
        known = skill;
        return true;
      }
    }

    return false;
  }
};

} // namespace fl::ecs::components
