#include "fl/ecs/systems/experience_award_policy.hpp"

namespace fl::systems {

int ExperienceAwardPolicy::for_character(int base_reward, int character_level,
                                         std::span<const int> defeated_enemy_levels,
                                         int repeated_clear_count) noexcept {
  (void)character_level;
  (void)defeated_enemy_levels;
  (void)repeated_clear_count;
  return base_reward;
}

} // namespace fl::systems
