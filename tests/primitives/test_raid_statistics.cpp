#include <array>
#include <catch2/catch_test_macros.hpp>
#include "fl/grand_central.hpp"
#include "fl/ecs/components/stats.hpp"

TEST_CASE("Raid records count collective outcomes once and commit before milestone notifications", "[raid][statistics][achievements]") {
  fl::GrandCentral gc{2, 5, 1};
  auto &milestones = gc.raid_milestones();
  int unlocks = 0;
  fl::events::ScopedAchievementListener listener{milestones.bus(),
      std::in_place_type<fl::events::AchievementUnlocked>, [&](const auto &event) {
        ++unlocks;
        REQUIRE(event.achievement == fl::events::AchievementId::RaidMutualDestruction);
        REQUIRE(milestones.mutual_destruction_unlocked());
        REQUIRE(milestones.records().defeats == 1);
        REQUIRE(gc.account_context(0).account_data().raid_records().defeats == 1);
      }};
  for (std::size_t index = 0; index < 2; ++index) {
    auto ctx = gc.account_context(index);
    auto &account = ctx.account_data();
    const std::array enemies{fl::monster::MonsterKind::FieldMouse};
    REQUIRE(account.start_raid(ctx, enemies));
    REQUIRE(account.raid_records().attempts == 1);
    REQUIRE(account.raid_records().party_participations == 5);
    auto &raid = *account.raid();
    for (auto id : raid.encounter().defenders()) gc.reg().get<fl::ecs::components::Stats>(id).hp_ = 0;
    for (auto id : raid.encounter().attackers()) gc.reg().get<fl::ecs::components::Stats>(id).hp_ = 0;
    raid.tick();
    auto duplicate = fl::events::RaidResolved{raid.id(), account.account_id(), *raid.result(), 5, raid.combat_beats()};
    account.raid_bus().emit(duplicate);
    account.raid_bus().emit(fl::events::RaidRecorded{duplicate});
    REQUIRE(account.raid_records().defeats == 1);
    REQUIRE(account.raid_records().mutual_destructions == 1);
    REQUIRE(account.raid_records().wins == 0);
    REQUIRE(account.raid_records().party_victory_credits == 0);
  }
  REQUIRE(unlocks == 1);
  REQUIRE(milestones.records().completed == 2);
  REQUIRE(milestones.records().mutual_destructions == 2);
}

TEST_CASE("One raid win grants five party credits and cannot unlock mutual destruction", "[raid][statistics]") {
  fl::GrandCentral gc{1, 5, 1};
  auto ctx = gc.account_context(0);
  auto &account = ctx.account_data();
  const std::array enemies{fl::monster::MonsterKind::FieldMouse};
  REQUIRE(account.start_raid(ctx, enemies));
  gc.reg().get<fl::ecs::components::Stats>(account.party(0).members().front().member_id()).hp_ = 0;
  for (auto id : account.raid()->encounter().attackers()) gc.reg().get<fl::ecs::components::Stats>(id).hp_ = 0;
  account.raid()->tick();
  REQUIRE(account.raid_records().wins == 1);
  REQUIRE(account.raid_records().party_victory_credits == 5);
  REQUIRE(gc.raid_milestones().records().wins == 1);
  REQUIRE_FALSE(gc.raid_milestones().mutual_destruction_unlocked());
}

TEST_CASE("Progress subscriptions can be disabled for isolated demos", "[raid][statistics]") {
  fl::GrandCentral gc{1, 1, 1, false, false};
  gc.innervate_event_system();
  auto &account = gc.account_context(0).account_data();
  REQUIRE(account.in_raid());
  for (auto id : account.raid()->encounter().attackers()) gc.reg().get<fl::ecs::components::Stats>(id).hp_ = 0;
  account.raid()->tick();
  REQUIRE(account.raid_records().attempts == 0);
  REQUIRE(account.raid_records().wins == 0);
  REQUIRE(gc.raid_milestones().records().completed == 0);
}

TEST_CASE("The initial Visitor plays through to a recorded wipe without advancing account time", "[raid][integration]") {
  fl::GrandCentral gc{1, 5, 5};
  gc.innervate_event_system();
  auto &account = gc.account_context(0).account_data();
  for (int beat = 0; beat < 10000 && account.in_raid(); ++beat)
    gc.beat_bus().emit(seerin::Beat{});
  REQUIRE_FALSE(account.in_raid());
  REQUIRE(account.raid_records().defeats == 1);
  REQUIRE(account.raid_records().combat_beats > 0);
  REQUIRE(account.calendar().elapsed_beats() == 0);
  REQUIRE_FALSE(gc.raid_milestones().mutual_destruction_unlocked());
}
