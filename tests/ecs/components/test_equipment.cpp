#include <array>
#include <catch2/catch_test_macros.hpp>

#include <entt/entity/registry.hpp>

#include "fl/ecs/components/equipment.hpp"
#include "fl/generated/gear_content.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/loot/equipment_stat_policy.hpp"
#include "fl/loot/global_loot_table.hpp"
#include "fl/loot/unique_items.hpp"
#include "fl/primitives/random_hub.hpp"

TEST_CASE("Equipment derives its Lospec palette index from tier", "[ecs][components][equipment]") {
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

TEST_CASE("EquipmentBuilder creates equipment with its tier", "[loot][equipment]") {
  entt::registry reg;
  fl::loot::EquipmentBuilder builder{
      .slot = fl::loot::EquipmentSlot::cape,
      .armor_kind = fl::loot::ArmorKind::cloth,
      .tier = fl::loot::Tier::excellent,
      .name = "Verdant Cape",
  };

  auto item = builder.create(reg);
  const auto& equipment = reg.get<fl::ecs::components::Equipment>(item);

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
  REQUIRE(fl::loot::inventory_has_unique_item(reg, inventory, "boots_of_damp_authority"));
  REQUIRE_FALSE(fl::loot::inventory_has_unique_item(reg, inventory, "mouse_nibbled_cape"));
}

TEST_CASE("Generated special gear uses the existing EquipmentBuilder fields",
          "[generated][content][gear]") {
  using fl::loot::generated_content::GearId;
  const auto table = fl::loot::global_loot_table();
  const auto specials = table.special_items();
  REQUIRE(specials.size() == 7);
  REQUIRE(fl::loot::generated_content::definition(GearId::VillageGuardHide).weight.value == 0);

  const auto& boots = fl::loot::generated_content::definition(GearId::BootsOfDampAuthority);
  const auto& cape = fl::loot::generated_content::definition(GearId::MouseNibbledCape);
  REQUIRE(boots.weight.value == 20);
  REQUIRE(cape.weight.value == 10);
  REQUIRE(specials[0].item.unique_id == boots.item.unique_id);
  REQUIRE(specials[1].item.unique_id == cape.item.unique_id);
  entt::registry reg;
  const auto boot_entity = boots.item.create(reg);
  const auto cape_entity = cape.item.create(reg);
  const auto& boot = reg.get<fl::ecs::components::Equipment>(boot_entity);
  const auto& cloak = reg.get<fl::ecs::components::Equipment>(cape_entity);
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

TEST_CASE("Ordinary equipment stats are deterministic persistent item data",
          "[loot][equipment][stats][determinism]") {
  auto generate = [](std::uint64_t seed) {
    fl::primitives::RandomHub rng{seed};
    fl::loot::EquipmentBuilder item{.slot = fl::loot::EquipmentSlot::cape,
                                    .armor_kind = fl::loot::ArmorKind::cloth,
                                    .tier = fl::loot::Tier::fine,
                                    .name = "Fine Cape"};
    fl::loot::apply_ordinary_equipment_stats(item, rng, "origin_woodland_tower",
                                             "test.equipment.deterministic");
    return item;
  };
  const auto first = generate(4242);
  const auto second = generate(4242);
  REQUIRE(first.biome_id == "origin_woodland_tower");
  REQUIRE(first.attack == second.attack);
  REQUIRE(first.armor == second.armor);
  REQUIRE(first.max_hp == second.max_hp);
  REQUIRE(first.resistances.physical == second.resistances.physical);
  REQUIRE(first.resistances.magical == second.resistances.magical);
  REQUIRE(first.resistances.fire == second.resistances.fire);
  REQUIRE(first.resistances.ice == second.resistances.ice);
  REQUIRE(first.resistances.lightning == second.resistances.lightning);

  entt::registry reg;
  const auto entity = first.create(reg);
  const auto& stored = reg.get<fl::ecs::components::Equipment>(entity);
  const int budget = fl::loot::equipment_stat_budget(stored);
  const auto range = fl::loot::equipment_tier_budget_range(fl::loot::Tier::fine);
  REQUIRE(budget >= range.minimum);
  REQUIRE(budget <= range.maximum);
  REQUIRE(stored.biome_id() == "origin_woodland_tower");
}

TEST_CASE("Tier budget ranges increase and biome weights change distributions",
          "[loot][equipment][stats][biome]") {
  using fl::loot::Tier;
  const std::array tiers{Tier::worn,      Tier::plain,      Tier::sturdy, Tier::fine,
                         Tier::excellent, Tier::masterwork, Tier::mythic};
  for (std::size_t i = 1; i < tiers.size(); ++i) {
    const auto previous = fl::loot::equipment_tier_budget_range(tiers[i - 1]);
    const auto current = fl::loot::equipment_tier_budget_range(tiers[i]);
    REQUIRE(current.minimum >= previous.minimum);
    REQUIRE(current.maximum > previous.maximum);
  }

  struct Totals {
    int armor{};
    int fire{};
  };
  auto totals = [](std::string_view biome) {
    fl::primitives::RandomHub rng{7331};
    Totals result;
    for (int i = 0; i < 200; ++i) {
      fl::loot::EquipmentBuilder item{.slot = fl::loot::EquipmentSlot::cape,
                                      .armor_kind = fl::loot::ArmorKind::cloth,
                                      .tier = Tier::fine,
                                      .name = "Experiment Cape"};
      fl::loot::apply_ordinary_equipment_stats(item, rng, biome,
                                               "test.equipment.biome." + std::to_string(i));
      result.armor += item.armor;
      result.fire += item.resistances.fire;
    }
    return result;
  };
  const auto plains = totals("origin_plains_tower");
  const auto caves = totals("origin_caves_tower");
  const auto woodland = totals("origin_woodland_tower");
  const auto desert = totals("origin_desert_tower");
  REQUIRE(caves.armor > plains.armor);
  REQUIRE(desert.fire > woodland.fire);

  const auto* woodland_profile =
      fl::loot::generated_content::equipment_profile("origin_woodland_tower");
  const auto* caves_profile = fl::loot::generated_content::equipment_profile("origin_caves_tower");
  const auto* desert_profile =
      fl::loot::generated_content::equipment_profile("origin_desert_tower");
  REQUIRE(woodland_profile->kinds.armor == 80);
  REQUIRE(caves_profile->kinds.jewelry == 75);
  REQUIRE(desert_profile->kinds.weapon == 75);
}

TEST_CASE("Equipment upgrades preserve provenance and specialization in a higher budget",
          "[loot][equipment][upgrade]") {
  entt::registry reg;
  auto make = [&] {
    return fl::loot::EquipmentBuilder{.slot = fl::loot::EquipmentSlot::mainhand,
                                      .armor_kind = fl::loot::ArmorKind::none,
                                      .tier = fl::loot::Tier::plain,
                                      .name = "Plain Mainhand",
                                      .biome_id = "origin_desert_tower",
                                      .attack = 2}
        .create(reg);
  };
  const auto one = make();
  const auto two = make();
  const auto three = make();
  fl::primitives::RandomHub rng{123};
  const auto upgraded = fl::loot::upgraded_equipment_builder(
      reg.get<fl::ecs::components::Equipment>(one), reg.get<fl::ecs::components::Equipment>(two),
      reg.get<fl::ecs::components::Equipment>(three), fl::loot::Tier::sturdy, rng,
      "test.equipment.upgrade");
  REQUIRE(upgraded.biome_id == "origin_desert_tower");
  REQUIRE(upgraded.attack >= 4);
  REQUIRE(upgraded.attack <= 6);
  REQUIRE(upgraded.armor == 0);
  REQUIRE(upgraded.max_hp == 0);
}
