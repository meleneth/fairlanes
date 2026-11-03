#include <array>
#include <catch2/catch_test_macros.hpp>

#include <entt/entity/registry.hpp>

#include "fl/ecs/components/equipment.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/loot/unique_items.hpp"
#include "fl/loot/global_loot_table.hpp"
#include "fl/generated/gear_content.hpp"

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

TEST_CASE("Generated special gear uses the existing EquipmentBuilder fields",
          "[generated][content][gear]") {
  using fl::loot::generated_content::GearId;
  const auto table = fl::loot::global_loot_table();
  const auto specials = table.special_items();
  REQUIRE(specials.size() == 7);
  REQUIRE(fl::loot::generated_content::definition(GearId::VillageGuardHide)
              .weight.value == 0);

  const auto &boots = fl::loot::generated_content::definition(GearId::BootsOfDampAuthority);
  const auto &cape = fl::loot::generated_content::definition(GearId::MouseNibbledCape);
  REQUIRE(boots.weight.value == 20);
  REQUIRE(cape.weight.value == 10);
  REQUIRE(specials[0].item.unique_id == boots.item.unique_id);
  REQUIRE(specials[1].item.unique_id == cape.item.unique_id);
  entt::registry reg;
  const auto boot_entity = boots.item.create(reg);
  const auto cape_entity = cape.item.create(reg);
  const auto &boot = reg.get<fl::ecs::components::Equipment>(boot_entity);
  const auto &cloak = reg.get<fl::ecs::components::Equipment>(cape_entity);
  REQUIRE(boot.unique_id() == "boots_of_damp_authority");
  REQUIRE(boot.name() == "Boots of Damp Authority");
  REQUIRE(boot.slot() == fl::loot::EquipmentSlot::boots);
  REQUIRE(boot.armor_kind() == fl::loot::ArmorKind::leather);
  REQUIRE(boot.tier() == fl::loot::Tier::excellent);
  REQUIRE(cloak.unique_id() == "mouse_nibbled_cape");
  REQUIRE(cloak.name() == "Mouse-Nibbled Cape");
  REQUIRE(cloak.slot() == fl::loot::EquipmentSlot::cape);
  REQUIRE(cloak.armor_kind() == fl::loot::ArmorKind::cloth);
  REQUIRE(cloak.tier() == fl::loot::Tier::fine);
  REQUIRE(cloak.armor() == 1);
}
