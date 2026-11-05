#pragma once

#include <array>

namespace fl::loot {

enum class EquipmentSlot {
  chest,
  helm,
  pants,
  belt,
  boots,
  gloves,
  sleeves,
  cape,

  necklace,
  ring_1,
  ring_2,

  mainhand,
  offhand,

  knife,
};

inline constexpr std::array<EquipmentSlot, 14> kEquipmentSlots{
    EquipmentSlot::mainhand, EquipmentSlot::offhand,  EquipmentSlot::chest,
    EquipmentSlot::helm,     EquipmentSlot::pants,    EquipmentSlot::boots,
    EquipmentSlot::gloves,   EquipmentSlot::sleeves,  EquipmentSlot::belt,
    EquipmentSlot::cape,     EquipmentSlot::necklace, EquipmentSlot::ring_1,
    EquipmentSlot::ring_2,   EquipmentSlot::knife,
};

}; // namespace fl::loot
