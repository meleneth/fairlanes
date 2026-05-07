// global_loot_table.cpp
#include "global_loot_table.hpp"
#include "item_kind.hpp"
#include "special_item.hpp"
#include "tier.hpp"
#include "upgrade_table.hpp"

#include <string>

namespace fl::loot {

using fl::loot::ArmorKind;
using fl::loot::EquipmentSlot;

static std::string slot_name(EquipmentSlot slot) {
  switch (slot) {
  case EquipmentSlot::chest:
    return "Chestpiece";
  case EquipmentSlot::helm:
    return "Helm";
  case EquipmentSlot::pants:
    return "Pants";
  case EquipmentSlot::belt:
    return "Belt";
  case EquipmentSlot::boots:
    return "Boots";
  case EquipmentSlot::gloves:
    return "Gloves";
  case EquipmentSlot::sleeves:
    return "Sleeves";
  case EquipmentSlot::cape:
    return "Cape";
  default:
    return "Equipment";
  }
}

static std::string kind_name(ArmorKind kind) {
  switch (kind) {
  case ArmorKind::cloth:
    return "Cloth";
  case ArmorKind::leather:
    return "Leather";
  case ArmorKind::plate:
    return "Plate";
  case ArmorKind::none:
    return "";
  }
  return "";
}

static std::string tier_prefix(Tier tier) {
  switch (tier) {
  case Tier::worn:
    return "Worn";
  case Tier::plain:
    return "Plain";
  case Tier::sturdy:
    return "Sturdy";
  case Tier::fine:
    return "Fine";
  case Tier::excellent:
    return "Excellent";
  case Tier::masterwork:
    return "Masterwork";
  case Tier::mythic:
    return "Mythic";
  }
  return "Odd";
}

std::string LootTable::generated_name(EquipmentSlot slot, ArmorKind kind,
                                      Tier tier) {
  return tier_prefix(tier) + " " + kind_name(kind) + " " + slot_name(slot);
}

LootTable global_loot_table() {
  return LootTable{
      Weight{25}, // 25% chance anything drops

      WeightedTable<ItemKind>{{
          {Weight{80}, ItemKind::armor}, {Weight{15}, ItemKind::special},
          // weapon/jewelry can come later
      }},

      WeightedTable<EquipmentSlot>{{
          {Weight{15}, EquipmentSlot::chest},
          {Weight{10}, EquipmentSlot::helm},
          {Weight{10}, EquipmentSlot::pants},
          {Weight{10}, EquipmentSlot::belt},
          {Weight{15}, EquipmentSlot::boots},
          {Weight{10}, EquipmentSlot::gloves},
          {Weight{10}, EquipmentSlot::sleeves},
          {Weight{10}, EquipmentSlot::cape},
      }},

      WeightedTable<ArmorKind>{{
          {Weight{40}, ArmorKind::cloth},
          {Weight{35}, ArmorKind::leather},
          {Weight{25}, ArmorKind::plate},
      }},
      UpgradeTable<Tier>{{
          {Weight{35}, Tier::plain},
          {Weight{25}, Tier::sturdy},
          {Weight{18}, Tier::fine},
          {Weight{12}, Tier::excellent},
          {Weight{7}, Tier::masterwork},
          {Weight{2}, Tier::mythic},
      }},
      {
          SpecialItem{
              .weight = Weight{20},
              .item =
                  EquipmentBuilder{
                      .slot = EquipmentSlot::boots,
                      .armor_kind = ArmorKind::leather,
                      .name = "Boots of Damp Authority",
                  },
          },
          SpecialItem{
              .weight = Weight{10},
              .item =
                  EquipmentBuilder{
                      .slot = EquipmentSlot::cape,
                      .armor_kind = ArmorKind::cloth,
                      .name = "Mouse-Nibbled Cape",
                  },
          },
      }};
}

} // namespace fl::loot
