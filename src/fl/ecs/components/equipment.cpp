// equipment.cpp
#include "equipment.hpp"

#include <utility>

namespace fl::ecs::components {

Equipment::Equipment(fl::loot::EquipmentSlot slot, std::string name,
                     fl::loot::ArmorKind armor_kind,
                     fl::loot::Tier tier)
    : slot_(slot), armor_kind_(armor_kind), tier_(tier),
      name_(std::move(name)) {}

fl::loot::EquipmentSlot Equipment::slot() const noexcept { return slot_; }

fl::loot::ArmorKind Equipment::armor_kind() const noexcept {
  return armor_kind_;
}

fl::loot::Tier Equipment::tier() const noexcept { return tier_; }

bool Equipment::is_armor() const noexcept {
  return armor_kind_ != fl::loot::ArmorKind::none;
}

std::string_view Equipment::name() const noexcept { return name_; }

std::size_t Equipment::palette_index() const noexcept {
  using fl::loot::Tier;

  switch (tier_) {
  case Tier::worn:
    return 32;
  case Tier::plain:
    return 24;
  case Tier::sturdy:
    return 23;
  case Tier::fine:
    return 29;
  case Tier::excellent:
    return 15;
  case Tier::masterwork:
    return 36;
  case Tier::mythic:
    return 37;
  }

  return 32;
}

std::string_view to_string(fl::loot::EquipmentSlot slot) {
  using fl::loot::EquipmentSlot;

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
