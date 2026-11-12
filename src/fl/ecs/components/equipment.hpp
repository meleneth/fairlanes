// equipment.hpp
#pragma once

#include <string>
#include <string_view>

#include "fl/loot/armor_kind.hpp"
#include "fl/loot/equipment_slot.hpp"
#include "fl/loot/tier.hpp"
#include "fl/primitives/damage.hpp"

namespace fl::ecs::components {

class Equipment {
public:
  Equipment(fl::loot::EquipmentSlot slot, std::string name,
            fl::loot::ArmorKind armor_kind = fl::loot::ArmorKind::none,
            fl::loot::Tier tier = fl::loot::Tier::worn, std::string unique_id = {},
            fl::primitives::Resistances resistances = {}, int attack = 0, int armor = 0,
            int max_hp = 0, std::string biome_id = {});

  [[nodiscard]] fl::loot::EquipmentSlot slot() const noexcept;
  [[nodiscard]] fl::loot::ArmorKind armor_kind() const noexcept;
  [[nodiscard]] fl::loot::Tier tier() const noexcept;
  [[nodiscard]] bool is_armor() const noexcept;
  [[nodiscard]] std::string_view name() const noexcept;
  [[nodiscard]] std::string_view unique_id() const noexcept;
  [[nodiscard]] bool is_unique() const noexcept;
  [[nodiscard]] std::string_view biome_id() const noexcept;
  [[nodiscard]] const fl::primitives::Resistances& resistances() const noexcept;
  [[nodiscard]] int attack() const noexcept {
    return attack_;
  }
  [[nodiscard]] int armor() const noexcept {
    return armor_;
  }
  [[nodiscard]] int max_hp() const noexcept {
    return max_hp_;
  }
  [[nodiscard]] std::size_t palette_index() const noexcept;

private:
  fl::loot::EquipmentSlot slot_;
  fl::loot::ArmorKind armor_kind_{fl::loot::ArmorKind::none};
  fl::loot::Tier tier_{fl::loot::Tier::worn};
  std::string name_;
  std::string unique_id_;
  std::string biome_id_;
  fl::primitives::Resistances resistances_{};
  int attack_{0};
  int armor_{0};
  int max_hp_{0};
};

std::string_view to_string(fl::loot::EquipmentSlot slot);

} // namespace fl::ecs::components
