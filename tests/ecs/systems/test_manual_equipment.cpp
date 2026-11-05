#include <algorithm>
#include <optional>
#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/generated/gear_content.hpp"
#include "fl/grand_central.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/skills/thump.hpp"

namespace {

entt::entity make_item(entt::registry& reg, fl::loot::EquipmentSlot slot, std::string name) {
  return fl::loot::EquipmentBuilder{
      .slot = slot,
      .armor_kind = fl::loot::ArmorKind::cloth,
      .tier = fl::loot::Tier::plain,
      .name = std::move(name),
  }
      .create(reg);
}

bool inventory_contains(const fl::primitives::PartyData& party, entt::entity item) {
  return std::find(party.items().begin(), party.items().end(), item) != party.items().end();
}

} // namespace

TEST_CASE("Manual equipment requests swap a validated party inventory item",
          "[party][equipment][events]") {
  fl::GrandCentral gc{1, 1, 1};
  auto party_ctx = gc.account_context(0).party_context(0);
  auto& party = party_ctx.party_data();
  auto& reg = party_ctx.reg();
  party.replace_items({});

  const auto member = party.members().front().member_id();
  auto& closet = reg.get<fl::ecs::components::PartyMember>(member).closet();
  const auto previous = make_item(reg, fl::loot::EquipmentSlot::chest, "Previous Chest");
  const auto replacement = make_item(reg, fl::loot::EquipmentSlot::chest, "Replacement Chest");
  closet.chest = previous;
  party.add_item(replacement);

  std::optional<fl::events::EquipmentEquipped> equipped;
  auto result_sub = party.party_bus().subscribe<fl::events::EquipmentEquipped>(
      [&](const auto& event) { equipped = event; });

  party.party_bus().emit(fl::events::PartyEvent{
      fl::events::EquipmentEquipRequested{member, fl::loot::EquipmentSlot::chest, replacement}});

  REQUIRE(equipped.has_value());
  REQUIRE(equipped->member == member);
  REQUIRE(equipped->item == replacement);
  REQUIRE(equipped->previous == previous);
  REQUIRE(closet.chest == replacement);
  REQUIRE_FALSE(inventory_contains(party, replacement));
  REQUIRE(inventory_contains(party, previous));
}

TEST_CASE("Manual equipment requests reject mismatched slots", "[party][equipment][events]") {
  fl::GrandCentral gc{1, 1, 1};
  auto party_ctx = gc.account_context(0).party_context(0);
  auto& party = party_ctx.party_data();
  auto& reg = party_ctx.reg();
  party.replace_items({});

  const auto member = party.members().front().member_id();
  auto& closet = reg.get<fl::ecs::components::PartyMember>(member).closet();
  const auto helm = make_item(reg, fl::loot::EquipmentSlot::helm, "A Helm");
  party.add_item(helm);

  bool emitted = false;
  auto result_sub = party.party_bus().subscribe<fl::events::EquipmentEquipped>(
      [&](const auto&) { emitted = true; });
  party.party_bus().emit(fl::events::PartyEvent{
      fl::events::EquipmentEquipRequested{member, fl::loot::EquipmentSlot::chest, helm}});

  REQUIRE_FALSE(emitted);
  REQUIRE(closet.chest == static_cast<entt::entity>(entt::null));
  REQUIRE(inventory_contains(party, helm));
}

TEST_CASE("Swapping stat gear recomputes bonuses without stacking or healing",
          "[party][equipment][stats]") {
  fl::GrandCentral gc{1, 1, 1};
  auto ctx = gc.account_context(0).party_context(0);
  auto& party = ctx.party_data();
  auto& reg = ctx.reg();
  const auto hero = party.members().front().member_id();
  auto& stats = reg.get<fl::ecs::components::Stats>(hero);
  stats.hp_ = stats.max_hp_ = 10;
  stats.attack_ = stats.armor_ = 0;
  party.replace_items({});
  const auto plain =
      fl::loot::EquipmentBuilder{.slot = fl::loot::EquipmentSlot::cape, .name = "Plain", .armor = 1}
          .create(reg);
  const auto improved = fl::loot::EquipmentBuilder{.slot = fl::loot::EquipmentSlot::cape,
                                                   .name = "Improved",
                                                   .attack = 2,
                                                   .armor = 3,
                                                   .max_hp = 5}
                            .create(reg);
  party.add_item(plain);
  party.add_item(improved);
  auto equip = [&](entt::entity item) {
    party.party_bus().emit(fl::events::PartyEvent{
        fl::events::EquipmentEquipRequested{hero, fl::loot::EquipmentSlot::cape, item}});
  };
  equip(plain);
  REQUIRE(fl::ecs::systems::effective_combat_stats(reg, hero).armor == 1);
  equip(improved);
  auto effective = fl::ecs::systems::effective_combat_stats(reg, hero);
  REQUIRE(effective.attack == 2);
  REQUIRE(effective.armor == 3);
  REQUIRE(effective.max_hp == 15);
  REQUIRE(stats.hp_ == 10); // Equipping does not heal.
  party.revitalize_members();
  REQUIRE(stats.hp_ == 15);
  equip(plain);
  REQUIRE(stats.hp_ == 10); // Removing Max HP clamps current HP.
  equip(improved);
  effective = fl::ecs::systems::effective_combat_stats(reg, hero);
  REQUIRE(effective.attack == 2);
  REQUIRE(effective.armor == 3);
  REQUIRE(effective.max_hp == 15);
  REQUIRE(stats.hp_ == 10);
}

TEST_CASE("Generated Attack and Armor gear reach production hit resolution",
          "[party][equipment][combat]") {
  auto thump_damage = [](bool with_stick) {
    fl::GrandCentral game{1, 1, 1, false, false, 42};
    auto ctx = game.account_context(0).party_context(0);
    auto& party = ctx.party_data();
    const auto hero = party.members().front().member_id();
    auto& fight =
        fl::primitives::EncounterBuilder{ctx}.one_on_one(fl::monster::MonsterKind::FieldMouse);
    const auto mouse = fight.attackers().members().front();
    auto& target = ctx.reg().get<fl::ecs::components::Stats>(mouse);
    target.hp_ = target.max_hp_ = 100;
    auto& source = ctx.reg().get<fl::ecs::components::Stats>(hero);
    source.attack_ = 0;
    if (with_stick) {
      const auto item = fl::loot::generated_content::definition(
                            fl::loot::generated_content::GearId::WoodlandStick)
                            .item.create(ctx.reg());
      party.add_item(item);
      party.party_bus().emit(fl::events::PartyEvent{
          fl::events::EquipmentEquipRequested{hero, fl::loot::EquipmentSlot::mainhand, item}});
    }
    fl::skills::Thump thump;
    return thump.thump(fl::context::AttackCtx::make_attack(ctx, hero, mouse));
  };
  REQUIRE(thump_damage(true) == thump_damage(false) + 2);

  auto incoming_damage = [](bool with_hide) {
    fl::GrandCentral game{1, 1, 1, false, false, 42};
    auto ctx = game.account_context(0).party_context(0);
    auto& party = ctx.party_data();
    const auto hero = party.members().front().member_id();
    auto& fight =
        fl::primitives::EncounterBuilder{ctx}.one_on_one(fl::monster::MonsterKind::FieldMouse);
    const auto mouse = fight.attackers().members().front();
    auto& target = ctx.reg().get<fl::ecs::components::Stats>(hero);
    target.hp_ = target.max_hp_ = 20;
    target.armor_ = 0;
    if (with_hide) {
      const auto item =
          fl::loot::generated_content::definition(fl::loot::generated_content::GearId::StitchedHide)
              .item.create(ctx.reg());
      party.add_item(item);
      party.party_bus().emit(fl::events::PartyEvent{
          fl::events::EquipmentEquipRequested{hero, fl::loot::EquipmentSlot::chest, item}});
    }
    auto hit = fl::context::AttackCtx::make_attack(ctx, mouse, hero);
    hit.damage().physical = 5;
    hit.set_hit_kind(fl::primitives::HitKind::Direct);
    return fl::ecs::systems::TakeDamage::commit(hit);
  };
  REQUIRE(incoming_damage(false) == 5);
  REQUIRE(incoming_damage(true) == 3);
}

TEST_CASE("Max HP gear permits survival of a heavy direct hit", "[party][equipment][combat]") {
  auto hp_after = [](bool with_locket) {
    fl::GrandCentral game{1, 1, 1, false, false, 42};
    auto ctx = game.account_context(0).party_context(0);
    auto& party = ctx.party_data();
    const auto hero = party.members().front().member_id();
    auto& fight =
        fl::primitives::EncounterBuilder{ctx}.one_on_one(fl::monster::MonsterKind::FieldMouse);
    const auto mouse = fight.attackers().members().front();
    auto& stats = ctx.reg().get<fl::ecs::components::Stats>(hero);
    stats.hp_ = stats.max_hp_ = 10;
    if (with_locket) {
      const auto item =
          fl::loot::generated_content::definition(fl::loot::generated_content::GearId::HeartyLocket)
              .item.create(ctx.reg());
      party.add_item(item);
      party.party_bus().emit(fl::events::PartyEvent{
          fl::events::EquipmentEquipRequested{hero, fl::loot::EquipmentSlot::necklace, item}});
      party.revitalize_members();
    }
    auto hit = fl::context::AttackCtx::make_attack(ctx, mouse, hero);
    hit.damage().physical = 12;
    hit.set_hit_kind(fl::primitives::HitKind::Direct);
    fl::ecs::systems::TakeDamage::commit(hit);
    return stats.hp_;
  };
  REQUIRE(hp_after(false) == 0);
  REQUIRE(hp_after(true) == 3);
}
