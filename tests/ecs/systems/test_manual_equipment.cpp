#include <algorithm>
#include <optional>
#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/grand_central.hpp"
#include "fl/loot/equipment_builder.hpp"

namespace {

entt::entity make_item(entt::registry &reg, fl::loot::EquipmentSlot slot,
                       std::string name) {
  return fl::loot::EquipmentBuilder{
      .slot = slot,
      .armor_kind = fl::loot::ArmorKind::cloth,
      .tier = fl::loot::Tier::plain,
      .name = std::move(name),
  }
      .create(reg);
}

bool inventory_contains(const fl::primitives::PartyData &party,
                        entt::entity item) {
  return std::find(party.items().begin(), party.items().end(), item) !=
         party.items().end();
}

} // namespace

TEST_CASE("Manual equipment requests swap a validated party inventory item",
          "[party][equipment][events]") {
  fl::GrandCentral gc{1, 1, 1};
  auto party_ctx = gc.account_context(0).party_context(0);
  auto &party = party_ctx.party_data();
  auto &reg = party_ctx.reg();
  party.replace_items({});

  const auto member = party.members().front().member_id();
  auto &closet = reg.get<fl::ecs::components::PartyMember>(member).closet();
  const auto previous =
      make_item(reg, fl::loot::EquipmentSlot::chest, "Previous Chest");
  const auto replacement =
      make_item(reg, fl::loot::EquipmentSlot::chest, "Replacement Chest");
  closet.chest = previous;
  party.add_item(replacement);

  std::optional<fl::events::EquipmentEquipped> equipped;
  auto result_sub = party.party_bus().subscribe<fl::events::EquipmentEquipped>(
      [&](const auto &event) { equipped = event; });

  party.party_bus().emit(
      fl::events::PartyEvent{fl::events::EquipmentEquipRequested{
          member, fl::loot::EquipmentSlot::chest, replacement}});

  REQUIRE(equipped.has_value());
  REQUIRE(equipped->member == member);
  REQUIRE(equipped->item == replacement);
  REQUIRE(equipped->previous == previous);
  REQUIRE(closet.chest == replacement);
  REQUIRE_FALSE(inventory_contains(party, replacement));
  REQUIRE(inventory_contains(party, previous));
}

TEST_CASE("Manual equipment requests reject mismatched slots",
          "[party][equipment][events]") {
  fl::GrandCentral gc{1, 1, 1};
  auto party_ctx = gc.account_context(0).party_context(0);
  auto &party = party_ctx.party_data();
  auto &reg = party_ctx.reg();
  party.replace_items({});

  const auto member = party.members().front().member_id();
  auto &closet = reg.get<fl::ecs::components::PartyMember>(member).closet();
  const auto helm = make_item(reg, fl::loot::EquipmentSlot::helm, "A Helm");
  party.add_item(helm);

  bool emitted = false;
  auto result_sub = party.party_bus().subscribe<fl::events::EquipmentEquipped>(
      [&](const auto &) { emitted = true; });
  party.party_bus().emit(
      fl::events::PartyEvent{fl::events::EquipmentEquipRequested{
          member, fl::loot::EquipmentSlot::chest, helm}});

  REQUIRE_FALSE(emitted);
  REQUIRE(closet.chest == static_cast<entt::entity>(entt::null));
  REQUIRE(inventory_contains(party, helm));
}
