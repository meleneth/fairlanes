#pragma once

#include "fl/consumables/consumable_kind.hpp"

namespace fl::ecs::components {

struct Consumable {
  fl::consumables::ConsumableKind kind{fl::consumables::ConsumableKind::HealingPotion};
  int quantity{1};
  int magnitude{0};
  int duration_seconds{0};
};

} // namespace fl::ecs::components
