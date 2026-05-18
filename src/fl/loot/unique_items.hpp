#pragma once

#include <span>
#include <string_view>

#include <entt/entt.hpp>

#include "fl/ecs/components/equipment.hpp"

namespace fl::loot {

inline bool inventory_has_unique_item(entt::registry &reg,
                                      std::span<const entt::entity> inventory,
                                      std::string_view unique_id) {
  if (unique_id.empty()) {
    return false;
  }

  for (auto item : inventory) {
    if (item == entt::null || !reg.valid(item)) {
      continue;
    }

    const auto *equipment = reg.try_get<fl::ecs::components::Equipment>(item);
    if (equipment != nullptr && equipment->unique_id() == unique_id) {
      return true;
    }
  }

  return false;
}

} // namespace fl::loot
