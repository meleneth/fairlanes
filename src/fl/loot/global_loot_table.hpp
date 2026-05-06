#pragma once

#include "loot_table.hpp"

namespace fl::loot {
LootTable global_loot_table() {
  using namespace fl::ecs::components;

  return LootTable{{
      LootEntry{
          .weight = Weight{25},
          .make = [] {
            return EquipmentBuilder{
                .slot = EquipmentSlot::boots,
                .armor_kind = ArmorKind::leather,
                .name = "Boots of Damp Authority",
            };
          }},
      LootEntry{
          .weight = Weight{10},
          .make = [] {
            return EquipmentBuilder{
                .slot = EquipmentSlot::cape,
                .armor_kind = ArmorKind::cloth,
                .name = "Mouse-Nibbled Cape",
            };
          }},
  }};
}
};
