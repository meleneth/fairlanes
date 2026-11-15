#pragma once

#include <array>

namespace fl::loot {

// Closed mechanical vocabulary. Ruby owns the balance policy for these stats;
// adding a new kind changes runtime mechanics and intentionally requires C++.
enum class EquipmentStat {
  attack,
  armor,
  max_hp,
  physical,
  magical,
  fire,
  ice,
  lightning,
};

inline constexpr std::array<EquipmentStat, 8> kEquipmentStats{
    EquipmentStat::attack,  EquipmentStat::armor, EquipmentStat::max_hp, EquipmentStat::physical,
    EquipmentStat::magical, EquipmentStat::fire,  EquipmentStat::ice,    EquipmentStat::lightning};

} // namespace fl::loot
