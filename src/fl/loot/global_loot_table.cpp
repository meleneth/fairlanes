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
  case EquipmentSlot::necklace:
    return "Necklace";
  case EquipmentSlot::ring_1:
  case EquipmentSlot::ring_2:
    return "Ring";
  case EquipmentSlot::mainhand:
    return "Mainhand";
  case EquipmentSlot::offhand:
    return "Offhand";
  case EquipmentSlot::knife:
    return "Knife";
  }

  return "Equipment";
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
  if (kind == ArmorKind::none) {
    return tier_prefix(tier) + " " + slot_name(slot);
  }

  return tier_prefix(tier) + " " + kind_name(kind) + " " + slot_name(slot);
}

LootTable global_loot_table() {
  return LootTable{
      Weight{25}, // 25% chance anything drops

      WeightedTable<ItemKind>{{
          {Weight{60}, ItemKind::armor},
          {Weight{20}, ItemKind::weapon},
          {Weight{15}, ItemKind::jewelry},
          {Weight{5}, ItemKind::special},
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
      WeightedTable<EquipmentSlot>{{
          {Weight{45}, EquipmentSlot::mainhand},
          {Weight{35}, EquipmentSlot::offhand},
          {Weight{20}, EquipmentSlot::knife},
      }},

      WeightedTable<EquipmentSlot>{{
          {Weight{34}, EquipmentSlot::necklace},
          {Weight{33}, EquipmentSlot::ring_1},
          {Weight{33}, EquipmentSlot::ring_2},
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
                      .tier = Tier::excellent,
                      .name = "Boots of Damp Authority",
                  },
          },
          SpecialItem{
              .weight = Weight{10},
              .item =
                  EquipmentBuilder{
                      .slot = EquipmentSlot::cape,
                      .armor_kind = ArmorKind::cloth,
                      .tier = Tier::fine,
                      .name = "Mouse-Nibbled Cape",
                  },
          },
      }};
}

} // namespace fl::loot
