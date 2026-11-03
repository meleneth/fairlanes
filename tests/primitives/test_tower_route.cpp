#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/generated/floor_content.hpp"
#include "fl/grand_central.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/encounter_builder.hpp"

TEST_CASE("Woodland floor nine unlocks Plains, whose first clear rewards the cape",
          "[tower][progression][reward]") {
  fl::GrandCentral game{1, 1, 5};
  auto ctx = game.account_context(0).party_context(0);
  auto &party = ctx.party_data();
  party.replace_items({});
  REQUIRE(party.select_tower("origin_woodland_tower"));
  REQUIRE_FALSE(party.select_tower("origin_plains_tower"));
  int plains_unlock_events = 0;
  fl::events::ScopedPartyListener unlock_sub{
      party.party_bus(), std::in_place_type<fl::events::BiomeUnlocked>,
      [&](const fl::events::BiomeUnlocked &event) {
        REQUIRE(event.biome_id == "origin_plains_tower");
        ++plains_unlock_events;
      }};

  auto clear_current_floor = [&] {
    const auto route = party.tower_route();
    REQUIRE(route.has_value());
    const auto *lineup = fl::primitives::generated_content::floor_lineup(
        route->biome_id, route->floor);
    REQUIRE(lineup != nullptr);
    party.begin_tower_encounter();
    auto &fight = fl::primitives::EncounterBuilder{
        ctx, fl::primitives::EncounterMode::ChaosAttractor,
        fl::primitives::EncounterBuilder::TowerFloor{
            std::string(route->biome_id), route->floor}}.thump_it_out();
    REQUIRE(fight.attackers().members().size() == lineup->monsters.size());
    // Resolve the setup without waiting for combat; the victory and reward
    // event path is the same one used after production damage kills a lineup.
    for (auto enemy : fight.attackers().members())
      ctx.reg().get<fl::ecs::components::Stats>(enemy).hp_ = 0;
    party.leave_combat();
  };

  for (int floor = 1; floor <= 9; ++floor) {
    REQUIRE(party.tower_route()->floor == floor);
    clear_current_floor();
  }
  REQUIRE(party.plains_unlocked());
  REQUIRE(plains_unlock_events == 1);
  REQUIRE(party.tower_route()->floor == 10);
  REQUIRE(party.select_tower("origin_plains_tower"));
  REQUIRE(party.tower_route()->floor == 1);
  clear_current_floor();
  REQUIRE(party.tower_route()->floor == 2);
  const auto reward = std::find_if(
      party.items().begin(), party.items().end(), [&](entt::entity item) {
        const auto *gear = ctx.reg().try_get<fl::ecs::components::Equipment>(item);
        return gear && gear->unique_id() == "mossbound_cape";
      });
  REQUIRE(reward != party.items().end());
  const auto hero = party.members().front().member_id();
  party.party_bus().emit(fl::events::PartyEvent{
      fl::events::EquipmentEquipRequested{
          hero, fl::loot::EquipmentSlot::cape, *reward}});
  REQUIRE(fl::ecs::systems::effective_combat_stats(ctx.reg(), hero).armor >= 3);
  REQUIRE(party.select_tower("origin_woodland_tower"));
  REQUIRE(party.tower_route()->floor == 10);
  const auto *floor_ten = fl::primitives::generated_content::floor_lineup(
      party.tower_route()->biome_id, party.tower_route()->floor);
  REQUIRE(floor_ten != nullptr);
  REQUIRE(floor_ten->monsters.front() == fl::monster::MonsterKind::HoneyBadger);
}

TEST_CASE("Seeded production beats can clear Woodland nine and take the Plains detour",
          "[tower][progression][combat]") {
  fl::GrandCentral game{1, 1, 5, false, false, 42};
  auto ctx = game.account_context(0).party_context(0);
  auto &party = ctx.party_data();
  party.replace_items({});
  for (auto &member : party.members()) {
    const auto hero = member.member_id();
    auto &stats = ctx.reg().get<fl::ecs::components::Stats>(hero);
    stats.hp_ = stats.max_hp_ = 20;
    stats.attack_ = 2;
    stats.armor_ = 0;
    auto &party_member = ctx.reg().get<fl::ecs::components::PartyMember>(hero);
    party_member.closet().unequip_skill(fl::skills::SkillId::Observe);
    member.grimoire().learn(fl::skills::SkillId::Thump);
    REQUIRE(party_member.equip_known_skill(fl::skills::SkillId::Thump));
  }
  REQUIRE(party.select_tower("origin_woodland_tower"));
  game.innervate_event_system(false);
  int beats = 0;
  while (party.tower_route()->floor < 10 && beats < 20000) {
    game.advance_beat();
    ++beats;
  }
  REQUIRE(party.tower_route()->floor == 10);
  REQUIRE(party.tower_choice_pending());
  REQUIRE(party.plains_unlocked());
  for (int wait = 0; wait < 12; ++wait) game.advance_beat();
  REQUIRE_FALSE(party.has_encounter());
  REQUIRE(party.tower_route()->floor == 10);
  REQUIRE(party.select_tower("origin_plains_tower"));
  while (party.tower_route()->floor < 2 && beats < 30000) {
    game.advance_beat();
    ++beats;
  }
  REQUIRE(party.tower_route()->floor == 2);
  const auto reward = std::find_if(
      party.items().begin(), party.items().end(), [&](entt::entity item) {
        const auto *gear = ctx.reg().try_get<fl::ecs::components::Equipment>(item);
        return gear && gear->unique_id() == "mossbound_cape";
      });
  REQUIRE(reward != party.items().end());
  const auto hero = party.members().front().member_id();
  party.party_bus().emit(fl::events::PartyEvent{
      fl::events::EquipmentEquipRequested{
          hero, fl::loot::EquipmentSlot::cape, *reward}});
  REQUIRE(fl::ecs::systems::effective_combat_stats(ctx.reg(), hero).armor >= 3);
  REQUIRE(party.select_tower("origin_woodland_tower"));
  REQUIRE(party.tower_route()->floor == 10);
}
