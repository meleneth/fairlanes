#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/raid_trinket.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/party_gearing.hpp"
#include "fl/grand_central.hpp"
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>

namespace {
std::size_t trinket_count(entt::registry &reg,
                          const fl::primitives::PartyData &party) {
  return std::ranges::count_if(party.items(), [&reg](auto item) {
    return reg.all_of<fl::ecs::components::RaidTrinket>(item);
  });
}
} // namespace

TEST_CASE("Raid victory grants exactly two exclusive trinkets to every party "
          "including wiped parties",
          "[raid][loot]") {
  fl::GrandCentral gc{1, 5, 1};
  auto ctx = gc.account_context(0);
  auto &account = ctx.account_data();
  const std::array enemies{fl::monster::MonsterKind::FieldMouse};
  REQUIRE(account.start_raid(ctx, enemies));
  auto &raid = *account.raid();
  gc.reg()
      .get<fl::ecs::components::Stats>(
          account.party(0).members().front().member_id())
      .hp_ = 0;
  int awards = 0;
  fl::events::ScopedRaidListener observer{
      account.raid_bus(), std::in_place_type<fl::events::RaidLootAwarded>,
      [&](const auto &event) {
        ++awards;
        REQUIRE(event.items == 10);
        for (const auto &party : account.parties())
          REQUIRE(trinket_count(gc.reg(), party) == 2);
      }};
  for (auto enemy : raid.encounter().attackers())
    gc.reg().get<fl::ecs::components::Stats>(enemy).hp_ = 0;
  raid.tick();
  raid.tick();
  REQUIRE(awards == 1);
  REQUIRE(account.raid_records().trinkets_awarded == 10);
  for (std::size_t i = 0; i < account.parties().size(); ++i) {
    auto party_ctx = ctx.party_context(i);
    fl::ecs::systems::PartyGearing::commit(party_ctx);
    REQUIRE(trinket_count(gc.reg(), account.party(i)) == 2);
    for (auto item : account.party(i).items()) {
      if (!gc.reg().all_of<fl::ecs::components::RaidTrinket>(item))
        continue;
      REQUIRE_FALSE(gc.reg().all_of<fl::ecs::components::Equipment>(item));
      REQUIRE(gc.reg().get<fl::ecs::components::RaidTrinket>(item).raid_id ==
              raid.id());
    }
  }
  account.raid_bus().emit(
      fl::events::RaidLootAwarded{raid.id(), account.account_id(), 10});
  REQUIRE(account.raid_records().trinkets_awarded == 10);
}

TEST_CASE("Raid defeat including mutual destruction grants no trinkets",
          "[raid][loot]") {
  for (bool mutual : {false, true}) {
    fl::GrandCentral gc{1, 2, 1};
    auto ctx = gc.account_context(0);
    auto &account = ctx.account_data();
    const std::array enemies{fl::monster::MonsterKind::FieldMouse};
    REQUIRE(account.start_raid(ctx, enemies));
    for (auto player : account.raid()->encounter().defenders())
      gc.reg().get<fl::ecs::components::Stats>(player).hp_ = 0;
    if (mutual)
      for (auto enemy : account.raid()->encounter().attackers())
        gc.reg().get<fl::ecs::components::Stats>(enemy).hp_ = 0;
    account.raid()->tick();
    REQUIRE(account.raid_records().trinkets_awarded == 0);
    for (auto &party : account.parties())
      REQUIRE(trinket_count(gc.reg(), party) == 0);
  }
}
