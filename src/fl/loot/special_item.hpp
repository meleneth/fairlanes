#pragma once

#include "equipment_builder.hpp"
#include "weight.hpp"

namespace fl::loot {

struct SpecialItem {
  Weight weight;
  EquipmentBuilder item;
};

} // namespace fl::loot
