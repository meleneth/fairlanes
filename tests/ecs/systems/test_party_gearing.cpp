#include <catch2/catch_test_macros.hpp>
#include <entt/entity/registry.hpp>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/systems/change_combat_affinity.hpp"
#include "fl/ecs/systems/party_gearing.hpp"
#include "fl/grand_central.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/primitives/party_data.hpp"

namespace {

entt::entity make_item(entt::registry& reg, fl::loot::EquipmentSlot slot, fl::loot::ArmorKind kind,
                       fl::loot::Tier tier, std::string name) {
  return fl::loot::EquipmentBuilder{
      .slot = slot,
      .armor_kind = kind,
      .tier = tier,
      .name = std::move(name),
  }
      .create(reg);
}

bool inventory_contains(const fl::primitives::PartyData& party, entt::entity item) {
  auto items = party.items();
  return std::find(items.begin(), items.end(), item) != items.end();
}

int inventory_count(const fl::primitives::PartyData& party, entt::entity item) {
  auto items = party.items();
  return static_cast<int>(std::count(items.begin(), items.end(), item));
}

} // namespace

TEST_CASE("PartyGearing equips the same armor kind already being worn",
          "[party][gearing][equipment]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto& party = party_ctx.party_data();
  auto& reg = party_ctx.reg();
  party.replace_items({});
  const auto member_id = party.members().front().member_id();
  auto& party_member = reg.get<fl::ecs::components::PartyMember>(member_id);
  reg.get<fl::ecs::components::TrackXP>(member_id).level_ = 100;
  auto& closet = party_member.closet();

  closet.chest = make_item(reg, fl::loot::EquipmentSlot::chest, fl::loot::ArmorKind::cloth,
                           fl::loot::Tier::plain, "Plain Cloth Chestpiece");

  auto leather_helm = make_item(reg, fl::loot::EquipmentSlot::helm, fl::loot::ArmorKind::leather,
                                fl::loot::Tier::mythic, "Mythic Leather Helm");
  auto cloth_helm = make_item(reg, fl::loot::EquipmentSlot::helm, fl::loot::ArmorKind::cloth,
                              fl::loot::Tier::plain, "Plain Cloth Helm");
  auto leather_mainhand =
      make_item(reg, fl::loot::EquipmentSlot::mainhand, fl::loot::ArmorKind::leather,
                fl::loot::Tier::mythic, "Mythic Leather Mainhand");
  auto cloth_mainhand =
      make_item(reg, fl::loot::EquipmentSlot::mainhand, fl::loot::ArmorKind::cloth,
                fl::loot::Tier::fine, "Fine Cloth Mainhand");
  auto necklace = make_item(reg, fl::loot::EquipmentSlot::necklace, fl::loot::ArmorKind::none,
                            fl::loot::Tier::excellent, "Excellent Necklace");
  party.add_item(leather_helm);
  party.add_item(cloth_helm);
  party.add_item(leather_mainhand);
  party.add_item(cloth_mainhand);
  party.add_item(necklace);

  const auto previous_log_size = party.log().size();
  fl::ecs::systems::PartyGearing::commit(party_ctx);

  REQUIRE(closet.helm == cloth_helm);
  REQUIRE(closet.mainhand == leather_mainhand);
  REQUIRE(closet.necklace == necklace);
  REQUIRE(inventory_contains(party, leather_helm));
  REQUIRE(inventory_contains(party, cloth_mainhand));
  REQUIRE_FALSE(inventory_contains(party, cloth_helm));
  REQUIRE_FALSE(inventory_contains(party, leather_mainhand));
  REQUIRE_FALSE(inventory_contains(party, necklace));
  REQUIRE(party.log().size() == previous_log_size + 3);
}

TEST_CASE("PartyGearing removes equipped items from loose inventory",
          "[party][gearing][equipment][inventory]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto& party = party_ctx.party_data();
  auto& reg = party_ctx.reg();
  party.replace_items({});
  const auto member_id = party.members().front().member_id();
  auto& party_member = reg.get<fl::ecs::components::PartyMember>(member_id);
  auto& closet = party_member.closet();

  closet.chest = make_item(reg, fl::loot::EquipmentSlot::chest, fl::loot::ArmorKind::cloth,
                           fl::loot::Tier::plain, "Plain Cloth Chestpiece");
  auto cloth_helm = make_item(reg, fl::loot::EquipmentSlot::helm, fl::loot::ArmorKind::cloth,
                              fl::loot::Tier::plain, "Plain Cloth Helm");

  party.add_item(cloth_helm);
  party.add_item(cloth_helm);

  fl::ecs::systems::PartyGearing::commit(party_ctx);

  REQUIRE(closet.helm == cloth_helm);
  REQUIRE(inventory_count(party, cloth_helm) == 0);
}

TEST_CASE("PartyGearing upgrades only loose normal gear", "[party][gearing][upgrade]") {
  fl::GrandCentral gc{1, 1, 0};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto& party = party_ctx.party_data();
  auto& reg = party_ctx.reg();
  party.replace_items({});

  for (int i = 0; i < 3; ++i) {
    party.add_item(make_item(reg, fl::loot::EquipmentSlot::gloves, fl::loot::ArmorKind::leather,
                             fl::loot::Tier::plain, "Plain Leather Gloves"));
    party.add_item(fl::loot::EquipmentBuilder{.slot = fl::loot::EquipmentSlot::cape,
                                              .armor_kind = fl::loot::ArmorKind::cloth,
                                              .tier = fl::loot::Tier::fine,
                                              .name = "Mouse-Nibbled Cape",
                                              .unique_id = "mouse_nibbled_cape"}
                       .create(reg));
    party.add_item(make_item(reg, fl::loot::EquipmentSlot::belt, fl::loot::ArmorKind::plate,
                             fl::loot::Tier::mythic, "Mythic Plate Belt"));
    party.add_item(make_item(reg, fl::loot::EquipmentSlot::mainhand, fl::loot::ArmorKind::cloth,
                             fl::loot::Tier::plain, "Plain Cloth Mainhand"));
    party.add_item(make_item(reg, fl::loot::EquipmentSlot::ring_1, fl::loot::ArmorKind::none,
                             fl::loot::Tier::plain, "Plain Ring"));
  }

  const auto previous_log_size = party.log().size();
  fl::ecs::systems::PartyGearing::commit(party_ctx);

  int sturdy_gloves = 0;
  int special_capes = 0;
  int mythic_belts = 0;
  int sturdy_mainhands = 0;
  int sturdy_rings = 0;
  for (auto item : party.items()) {
    const auto& equipment = reg.get<fl::ecs::components::Equipment>(item);
    if (equipment.slot() == fl::loot::EquipmentSlot::gloves &&
        equipment.armor_kind() == fl::loot::ArmorKind::leather &&
        equipment.tier() == fl::loot::Tier::sturdy) {
      ++sturdy_gloves;
    }
    if (equipment.name() == "Mouse-Nibbled Cape") {
      ++special_capes;
    }
    if (equipment.slot() == fl::loot::EquipmentSlot::belt &&
        equipment.tier() == fl::loot::Tier::mythic) {
      ++mythic_belts;
    }
    if (equipment.slot() == fl::loot::EquipmentSlot::mainhand &&
        equipment.armor_kind() == fl::loot::ArmorKind::cloth &&
        equipment.tier() == fl::loot::Tier::sturdy) {
      ++sturdy_mainhands;
    }
    if (equipment.slot() == fl::loot::EquipmentSlot::ring_1 &&
        equipment.armor_kind() == fl::loot::ArmorKind::none &&
        equipment.tier() == fl::loot::Tier::sturdy) {
      ++sturdy_rings;
    }
  }

  REQUIRE(sturdy_gloves == 1);
  REQUIRE(special_capes == 3);
  REQUIRE(mythic_belts == 3);
  REQUIRE(sturdy_mainhands == 1);
  REQUIRE(sturdy_rings == 1);
  REQUIRE(party.items().size() == 9);
  REQUIRE(party.log().size() == previous_log_size + 3);
}

TEST_CASE("PartyGearing preserves stronger specialization and reevaluates on level gain",
          "[party][gearing][stats][level]") {
  fl::GrandCentral gc{1, 1, 1};
  auto ctx = gc.account_context(0).party_context(0);
  auto& party = ctx.party_data();
  auto& reg = ctx.reg();
  party.replace_items({});
  const auto hero = party.members().front().member_id();
  auto& member = reg.get<fl::ecs::components::PartyMember>(hero);
  auto& xp = reg.get<fl::ecs::components::TrackXP>(hero);
  xp.level_ = 9;

  const auto specialized = fl::loot::EquipmentBuilder{.slot = fl::loot::EquipmentSlot::mainhand,
                                                      .tier = fl::loot::Tier::plain,
                                                      .name = "Specialized Plain Mainhand",
                                                      .attack = 4}
                               .create(reg);
  member.closet().mainhand = specialized;
  const auto eligible_later = fl::loot::EquipmentBuilder{.slot = fl::loot::EquipmentSlot::mainhand,
                                                         .tier = fl::loot::Tier::sturdy,
                                                         .name = "Sturdy Mainhand",
                                                         .attack = 5}
                                  .create(reg);
  const auto shifted_build = fl::loot::EquipmentBuilder{.slot = fl::loot::EquipmentSlot::mainhand,
                                                        .tier = fl::loot::Tier::plain,
                                                        .name = "Defensive Plain Mainhand",
                                                        .armor = 10}
                                 .create(reg);
  party.add_item(eligible_later);
  party.add_item(shifted_build);

  fl::ecs::systems::PartyGearing::commit(ctx);
  REQUIRE(member.closet().mainhand == specialized);
  REQUIRE(inventory_contains(party, eligible_later));
  REQUIRE(inventory_contains(party, shifted_build));

  xp.level_ = 10;
  member.member_data().bus().emit(
      fl::events::PartyEvent{fl::events::PartyGainedLevel{hero, xp.level_}});
  REQUIRE(member.closet().mainhand == eligible_later);
  REQUIRE(inventory_contains(party, specialized));
  REQUIRE(inventory_contains(party, shifted_build));
}

TEST_CASE("Town affinity changes replace compatible armor without losing ownership",
          "[party][gearing][affinity][inventory]") {
  fl::GrandCentral game{1, 1, 1};
  auto ctx = game.account_context(0).party_context(0);
  auto& party = ctx.party_data();
  auto& reg = ctx.reg();
  party.replace_items({});
  const auto hero = party.members().front().member_id();
  auto& member = reg.get<fl::ecs::components::PartyMember>(hero);

  const auto heirloom =
      fl::loot::EquipmentBuilder{
          .slot = fl::loot::EquipmentSlot::chest,
          .armor_kind = fl::loot::ArmorKind::cloth,
          .tier = fl::loot::Tier::plain,
          .name = "Remembered Cloth Chest",
          .unique_id = "remembered_cloth_chest",
          .attack = 3,
      }
          .create(reg);
  member.closet().chest = heirloom;
  const auto leather = make_item(reg, fl::loot::EquipmentSlot::chest, fl::loot::ArmorKind::leather,
                                 fl::loot::Tier::worn, "Worn Leather Chest");
  const auto locked_plate =
      make_item(reg, fl::loot::EquipmentSlot::chest, fl::loot::ArmorKind::plate,
                fl::loot::Tier::sturdy, "Sturdy Plate Chest");
  party.add_item(leather);
  party.add_item(locked_plate);

  REQUIRE(fl::ecs::systems::ChangeCombatAffinity::commit(ctx, hero,
                                                         fl::primitives::CombatAffinity::Leather) ==
          fl::ecs::systems::AffinityChangeResult::Changed);
  REQUIRE(member.closet().chest == leather);
  REQUIRE(inventory_count(party, heirloom) == 1);
  REQUIRE(inventory_count(party, leather) == 0);
  REQUIRE(inventory_count(party, locked_plate) == 1);
  REQUIRE(reg.valid(heirloom));
  REQUIRE(reg.get<fl::ecs::components::Equipment>(heirloom).unique_id() ==
          "remembered_cloth_chest");
  REQUIRE(reg.get<fl::ecs::components::Equipment>(heirloom).attack() == 3);

  REQUIRE(fl::ecs::systems::ChangeCombatAffinity::commit(ctx, hero,
                                                         fl::primitives::CombatAffinity::Plate) ==
          fl::ecs::systems::AffinityChangeResult::Changed);
  REQUIRE(member.closet().chest == leather);
  REQUIRE(inventory_count(party, heirloom) == 1);
  REQUIRE(inventory_count(party, locked_plate) == 1);
}
