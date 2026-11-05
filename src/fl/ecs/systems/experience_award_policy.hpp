#pragma once

#include <span>

namespace fl::systems {

// Narrow boundary for the future approved overlevel and repetition policies.
// The current implementation deliberately preserves the base encounter reward.
struct ExperienceAwardPolicy {
  static int for_character(int base_reward, int character_level,
                           std::span<const int> defeated_enemy_levels,
                           int repeated_clear_count) noexcept;
};

} // namespace fl::systems
