#include <catch2/catch_test_macros.hpp>
#include <entt/entt.hpp>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/systems/party_gearing.hpp"
#include "fl/grand_central.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/primitives/party_data.hpp"

namespace {

entt::entity make_item(entt::registry &reg, fl::loot::EquipmentSlot slot,
                       fl::loot::ArmorKind kind, fl::loot::Tier tier,
                       std::string name) {
  return fl::loot::EquipmentBuilder{
      .slot = slot,
      .armor_kind = kind,
      .tier = tier,
      .name = std::move(name),
  }.create(reg);
}

bool inventory_contains(const fl::primitives::PartyData &party,
                        entt::entity item) {
  auto items = party.items();
  return std::find(items.begin(), items.end(), item) != items.end();
}

} // namespace

TEST_CASE("PartyGearing equips the same armor kind already being worn",
          "[party][gearing][equipment]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &reg = party_ctx.reg();
  party.replace_items({});
  const auto member_id = party.members().front().member_id();
  auto &party_member = reg.get<fl::ecs::components::PartyMember>(member_id);
  auto &closet = party_member.closet();

  closet.chest =
      make_item(reg, fl::loot::EquipmentSlot::chest, fl::loot::ArmorKind::cloth,
                fl::loot::Tier::plain, "Plain Cloth Chestpiece");

  auto leather_helm =
      make_item(reg, fl::loot::EquipmentSlot::helm, fl::loot::ArmorKind::leather,
                fl::loot::Tier::mythic, "Mythic Leather Helm");
  auto cloth_helm =
      make_item(reg, fl::loot::EquipmentSlot::helm, fl::loot::ArmorKind::cloth,
                fl::loot::Tier::plain, "Plain Cloth Helm");
  party.add_item(leather_helm);
  party.add_item(cloth_helm);

  const auto previous_log_size = party.log().size();
  fl::ecs::systems::PartyGearing::commit(party_ctx);

  REQUIRE(closet.helm == cloth_helm);
  REQUIRE(inventory_contains(party, leather_helm));
  REQUIRE_FALSE(inventory_contains(party, cloth_helm));
  REQUIRE(party.log().size() == previous_log_size + 1);
}

TEST_CASE("PartyGearing upgrades only loose normal gear",
          "[party][gearing][upgrade]") {
  fl::GrandCentral gc{1, 1, 0};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &reg = party_ctx.reg();
  party.replace_items({});

  for (int i = 0; i < 3; ++i) {
    party.add_item(make_item(reg, fl::loot::EquipmentSlot::gloves,
                             fl::loot::ArmorKind::leather,
                             fl::loot::Tier::plain, "Plain Leather Gloves"));
    party.add_item(make_item(reg, fl::loot::EquipmentSlot::cape,
                             fl::loot::ArmorKind::cloth, fl::loot::Tier::fine,
                             "Mouse-Nibbled Cape"));
    party.add_item(make_item(reg, fl::loot::EquipmentSlot::belt,
                             fl::loot::ArmorKind::plate,
                             fl::loot::Tier::mythic, "Mythic Plate Belt"));
  }

  const auto previous_log_size = party.log().size();
  fl::ecs::systems::PartyGearing::commit(party_ctx);

  int sturdy_gloves = 0;
  int special_capes = 0;
  int mythic_belts = 0;
  for (auto item : party.items()) {
    const auto &equipment = reg.get<fl::ecs::components::Equipment>(item);
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
  }

  REQUIRE(sturdy_gloves == 1);
  REQUIRE(special_capes == 3);
  REQUIRE(mythic_belts == 3);
  REQUIRE(party.items().size() == 7);
  REQUIRE(party.log().size() == previous_log_size + 1);
}
