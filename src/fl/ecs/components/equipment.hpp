// equipment.hpp
#pragma once

#include <string>
#include <string_view>

#include "fl/loot/armor_kind.hpp"
#include "fl/loot/equipment_slot.hpp"

namespace fl::ecs::components {

class Equipment {
public:
  Equipment(fl::loot::EquipmentSlot slot, std::string name,
            fl::loot::ArmorKind armor_kind = fl::loot::ArmorKind::none);

  [[nodiscard]] fl::loot::EquipmentSlot slot() const noexcept;
  [[nodiscard]] fl::loot::ArmorKind armor_kind() const noexcept;
  [[nodiscard]] bool is_armor() const noexcept;
  [[nodiscard]] std::string_view name() const noexcept;

private:
  fl::loot::EquipmentSlot slot_;
  fl::loot::ArmorKind armor_kind_{fl::loot::ArmorKind::none};
  std::string name_;
};

std::string_view to_string(fl::loot::EquipmentSlot slot);

} // namespace fl::ecs::components
