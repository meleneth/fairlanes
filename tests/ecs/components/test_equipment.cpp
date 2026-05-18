#include <array>
#include <catch2/catch_test_macros.hpp>

#include <entt/entt.hpp>

#include "fl/ecs/components/equipment.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/loot/unique_items.hpp"

TEST_CASE("Equipment derives its Lospec palette index from tier",
          "[ecs][components][equipment]") {
  fl::ecs::components::Equipment fine_equipment{
      fl::loot::EquipmentSlot::boots,
      "Bright Boots",
      fl::loot::ArmorKind::leather,
      fl::loot::Tier::fine,
  };
  fl::ecs::components::Equipment mythic_equipment{
      fl::loot::EquipmentSlot::cape,
      "Wild Cape",
      fl::loot::ArmorKind::cloth,
      fl::loot::Tier::mythic,
  };

  REQUIRE(fine_equipment.tier() == fl::loot::Tier::fine);
  REQUIRE(fine_equipment.palette_index() == 29);

  REQUIRE(mythic_equipment.tier() == fl::loot::Tier::mythic);
  REQUIRE(mythic_equipment.palette_index() == 37);
}

TEST_CASE("EquipmentBuilder creates equipment with its tier",
          "[loot][equipment]") {
  entt::registry reg;
  fl::loot::EquipmentBuilder builder{
      .slot = fl::loot::EquipmentSlot::cape,
      .armor_kind = fl::loot::ArmorKind::cloth,
      .tier = fl::loot::Tier::excellent,
      .name = "Verdant Cape",
  };

  auto item = builder.create(reg);
  const auto &equipment = reg.get<fl::ecs::components::Equipment>(item);

  REQUIRE(equipment.name() == "Verdant Cape");
  REQUIRE(equipment.tier() == fl::loot::Tier::excellent);
  REQUIRE(equipment.palette_index() == 15);
}

TEST_CASE("Unique item helper detects existing party inventory uniques",
          "[loot][equipment][unique]") {
  entt::registry reg;
  fl::loot::EquipmentBuilder builder{
      .slot = fl::loot::EquipmentSlot::boots,
      .armor_kind = fl::loot::ArmorKind::leather,
      .tier = fl::loot::Tier::excellent,
      .name = "Boots of Damp Authority",
      .unique_id = "boots_of_damp_authority",
  };

  auto item = builder.create(reg);
  const std::array inventory{item};

  REQUIRE(reg.get<fl::ecs::components::Equipment>(item).is_unique());
  REQUIRE(fl::loot::inventory_has_unique_item(reg, inventory,
                                              "boots_of_damp_authority"));
  REQUIRE_FALSE(fl::loot::inventory_has_unique_item(reg, inventory,
                                                    "mouse_nibbled_cape"));
}
