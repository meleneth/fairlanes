#pragma once

#include <string_view>

#include "fl/loot/equipment_builder.hpp"
#include "fl/primitives/random_hub.hpp"

namespace fl::loot {

// Experimental Origin balance policy. Canonical design owns the separation of
// tier, slot, and biome; these particular budgets and weights do not.
struct EquipmentBudgetRange {
  int minimum;
  int maximum;
};

[[nodiscard]] EquipmentBudgetRange equipment_tier_budget_range(Tier tier) noexcept;
[[nodiscard]] int equipment_minimum_level(Tier tier) noexcept;
[[nodiscard]] inline bool can_equip(const fl::ecs::components::Equipment& item,
                                    int character_level) noexcept {
  return character_level >= equipment_minimum_level(item.tier());
}
[[nodiscard]] int equipment_stat_budget(const fl::ecs::components::Equipment& item) noexcept;
[[nodiscard]] int equipment_combat_value(const fl::ecs::components::Equipment& item) noexcept;

void apply_ordinary_equipment_stats(EquipmentBuilder& item, fl::primitives::RandomHub& rng,
                                    std::string_view biome_id, std::string_view stream_name);

// Keeps the inputs' relative specialization while increasing to the target
// tier budget. Inputs are expected to share slot, armor kind, tier, and biome.
[[nodiscard]] EquipmentBuilder
upgraded_equipment_builder(const fl::ecs::components::Equipment& first,
                           const fl::ecs::components::Equipment& second,
                           const fl::ecs::components::Equipment& third, Tier target_tier,
                           fl::primitives::RandomHub& rng, std::string_view stream_name);

} // namespace fl::loot
