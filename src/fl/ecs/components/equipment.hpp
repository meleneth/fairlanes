// equipment.hpp
#pragma once

#include <string>
#include <string_view>

namespace fl::ecs::components {

enum class ArmorKind {
  none,
  cloth,   // magic
  leather, // dex
  plate,   // str
};

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

class Equipment {
public:
  Equipment(EquipmentSlot slot, std::string name,
            ArmorKind armor_kind = ArmorKind::none);

  [[nodiscard]] EquipmentSlot slot() const noexcept;
  [[nodiscard]] ArmorKind armor_kind() const noexcept;
  [[nodiscard]] bool is_armor() const noexcept;
  [[nodiscard]] std::string_view name() const noexcept;

private:
  EquipmentSlot slot_;
  ArmorKind armor_kind_{ArmorKind::none};
  std::string name_;
};

std::string_view to_string(EquipmentSlot slot);

} // namespace fl::ecs::components
