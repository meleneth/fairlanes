#pragma once

#include <array>
#include <algorithm>

#include "fl/economy/profession.hpp"

namespace fl::ecs::components {

struct ProfessionExpertise {
  std::array<int, static_cast<std::size_t>(fl::economy::Profession::Count)> experience{};

  [[nodiscard]] int xp(fl::economy::Profession profession) const noexcept {
    return experience[static_cast<std::size_t>(profession)];
  }
  [[nodiscard]] int proficiency(fl::economy::Profession profession) const noexcept {
    return std::min(100, xp(profession) / 10);
  }
  int grant(fl::economy::Profession profession, int amount) noexcept {
    auto& current = experience[static_cast<std::size_t>(profession)];
    const int before = current;
    current = std::max(0, current + std::max(0, amount));
    return current - before;
  }
};

} // namespace fl::ecs::components
