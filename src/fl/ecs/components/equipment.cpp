// equipment.cpp
#include "equipment.hpp"

#include <utility>

namespace fl::ecs::components {

Equipment::Equipment(EquipmentSlot slot, std::string name,
                     ArmorKind armor_kind)
    : slot_(slot), armor_kind_(armor_kind), name_(std::move(name)) {}

EquipmentSlot Equipment::slot() const noexcept { return slot_; }

ArmorKind Equipment::armor_kind() const noexcept { return armor_kind_; }

bool Equipment::is_armor() const noexcept {
  return armor_kind_ != ArmorKind::none;
}

std::string_view Equipment::name() const noexcept { return name_; }

std::string_view to_string(EquipmentSlot slot) {
  switch (slot) {
  case EquipmentSlot::chest:
    return "chest";
  case EquipmentSlot::helm:
    return "helm";
  case EquipmentSlot::pants:
    return "pants";
  case EquipmentSlot::belt:
    return "belt";
  case EquipmentSlot::boots:
    return "boots";
  case EquipmentSlot::gloves:
    return "gloves";
  case EquipmentSlot::sleeves:
    return "sleeves";
  case EquipmentSlot::cape:
    return "cape";
  case EquipmentSlot::necklace:
    return "necklace";
  case EquipmentSlot::ring_1:
    return "ring_1";
  case EquipmentSlot::ring_2:
    return "ring_2";
  case EquipmentSlot::mainhand:
    return "mainhand";
  case EquipmentSlot::offhand:
    return "offhand";
  case EquipmentSlot::knife:
    return "knife";
  }

  return "unknown";
}

} // namespace fl::ecs::components
