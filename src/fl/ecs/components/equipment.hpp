// equipment.hpp
#pragma once

#include <string>
#include <string_view>

#include "fl/loot/armor_kind.hpp"
#include "fl/loot/equipment_slot.hpp"
#include "fl/loot/tier.hpp"

namespace fl::ecs::components {

class Equipment {
public:
  Equipment(fl::loot::EquipmentSlot slot, std::string name,
            fl::loot::ArmorKind armor_kind = fl::loot::ArmorKind::none,
            fl::loot::Tier tier = fl::loot::Tier::worn);

  [[nodiscard]] fl::loot::EquipmentSlot slot() const noexcept;
  [[nodiscard]] fl::loot::ArmorKind armor_kind() const noexcept;
  [[nodiscard]] fl::loot::Tier tier() const noexcept;
  [[nodiscard]] bool is_armor() const noexcept;
  [[nodiscard]] std::string_view name() const noexcept;
  [[nodiscard]] std::size_t palette_index() const noexcept;

private:
  fl::loot::EquipmentSlot slot_;
  fl::loot::ArmorKind armor_kind_{fl::loot::ArmorKind::none};
  fl::loot::Tier tier_{fl::loot::Tier::worn};
  std::string name_;
};

std::string_view to_string(fl::loot::EquipmentSlot slot);

} // namespace fl::ecs::components
