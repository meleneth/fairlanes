#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <ranges>
#include <string>
#include <vector>

#include "fl/ecs/components/track_xp.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/objective_system.hpp"

namespace {

constexpr std::array<std::string_view, 5> kOriginBiomes{
    "origin_woodland_tower", "origin_plains_tower", "origin_murky_swamp_tower",
    "origin_caves_tower", "origin_desert_tower"};

const fl::primitives::RecruitmentOpportunity&
opportunity(const fl::primitives::AccountData& account, std::string_view id) {
  const auto found = std::ranges::find(account.recruitment_opportunities(), id,
                                       &fl::primitives::RecruitmentOpportunity::id);
  REQUIRE(found != account.recruitment_opportunities().end());
  return *found;
}

void commit_floor(fl::primitives::AccountData& account, fl::context::AccountCtx ctx,
                  std::string_view biome, int floor) {
  account.parties().front().party_bus().emit(
      fl::events::PartyEvent{fl::events::TowerFloorCleared{std::string(biome), floor}});
  account.resolve_recruitment_events(ctx);
}

} // namespace

TEST_CASE("Story recruits consume capacity while excess floor-five opportunities remain available",
          "[recruitment][story][capacity][idempotency]") {
  fl::GrandCentral game{1, 1, 0, true, true, 42, true};
  auto ctx = game.account_context(0);
  auto& account = ctx.account_data();
  auto& bus = account.parties().front().party_bus();
  int opportunities = 0;
  int recruited = 0;
  int completed = 0;
  fl::events::ScopedPartyListener opportunity_sub{
      bus, std::in_place_type<fl::events::RecruitmentOpportunityUnlocked>,
      [&](const auto&) { ++opportunities; }};
  fl::events::ScopedPartyListener recruited_sub{bus, std::in_place_type<fl::events::HeroRecruited>,
                                                [&](const auto&) { ++recruited; }};
  fl::events::ScopedPartyListener completed_sub{
      bus, std::in_place_type<fl::events::PartyRosterCompleted>, [&](const auto&) { ++completed; }};

  REQUIRE(account.recruit_story_hero(ctx, "story_alderion", "Alderion"));
  REQUIRE(account.recruit_story_hero(ctx, "story_bramblethorn", "Bramblethorn"));
  REQUIRE_FALSE(account.recruit_story_hero(ctx, "story_alderion", "Alderion"));
  for (const auto biome : kOriginBiomes) commit_floor(account, ctx, biome, 5);
  for (const auto biome : kOriginBiomes) commit_floor(account, ctx, biome, 5);
  REQUIRE(opportunities == 5);
  commit_floor(account, ctx, "origin_woodland_tower", 25);
  REQUIRE(opportunity(account, "origin_woodland_floor_5_recruit").state ==
          fl::primitives::RecruitmentOpportunityState::Available);

  for (std::size_t index = 0; index < 3; ++index) {
    const auto biome = kOriginBiomes[index];
    const auto id = std::string(biome.substr(0, biome.size() - 6)) + "_floor_5_recruit";
    REQUIRE(account.claim_recruitment(ctx, id));
    REQUIRE_FALSE(account.claim_recruitment(ctx, id));
  }
  REQUIRE(account.parties().front().members().size() == 5);
  REQUIRE(account.parties().front().members()[2].name() == "Cyran Vale");
  REQUIRE(recruited == 5); // two story commits and three milestone commits
  REQUIRE(completed == 1);

  const auto caves = "origin_caves_floor_5_recruit";
  REQUIRE(opportunity(account, caves).state ==
          fl::primitives::RecruitmentOpportunityState::Available);
  REQUIRE_FALSE(account.claim_recruitment(ctx, caves));
  REQUIRE(opportunity(account, caves).state ==
          fl::primitives::RecruitmentOpportunityState::Available);
  REQUIRE(recruited == 5);

  const auto veteran = account.parties().front().members()[2].member_id();
  REQUIRE(account.request_roster_transfer(ctx, veteran, 1));
  REQUIRE(account.parties().front().members().size() == 4);
  REQUIRE(account.parties()[1].members().size() == 1);
  REQUIRE(account.claim_recruitment(ctx, caves));
  REQUIRE(account.parties().front().members().size() == 5);
  const auto fresh_recruit = account.parties().front().members().back().member_id();
  const auto& fresh_xp = ctx.reg().get<fl::ecs::components::TrackXP>(fresh_recruit);
  REQUIRE(fresh_xp.level_ == 1);
  REQUIRE(fresh_xp.xp_ == 0);
  REQUIRE(opportunity(account, caves).state ==
          fl::primitives::RecruitmentOpportunityState::Claimed);
}

TEST_CASE("Milestone tiers unlock parties independently and party four unlocks party five once",
          "[recruitment][milestones][party-five][ordering]") {
  fl::GrandCentral game{1, 1, 0, true, true, 7, true};
  auto ctx = game.account_context(0);
  auto& account = ctx.account_data();
  auto& bus = account.parties().front().party_bus();
  int opportunities = 0;
  int unlocked = 0;
  int completed = 0;
  bool fifth_party_batch_was_atomic = false;
  fl::events::ScopedPartyListener opportunity_sub{
      bus, std::in_place_type<fl::events::RecruitmentOpportunityUnlocked>, [&](const auto& event) {
        ++opportunities;
        if (event.opportunity_id == "origin_party_five_recruit_1") {
          const auto available =
              std::ranges::count_if(account.recruitment_opportunities(), [](const auto& entry) {
                return entry.destination_party == 4 &&
                       entry.state == fl::primitives::RecruitmentOpportunityState::Available;
              });
          fifth_party_batch_was_atomic = available == 5;
        }
      }};
  fl::events::ScopedPartyListener unlocked_sub{bus, std::in_place_type<fl::events::PartyUnlocked>,
                                               [&](const auto&) { ++unlocked; }};
  fl::events::ScopedPartyListener completed_sub{
      bus, std::in_place_type<fl::events::PartyRosterCompleted>, [&](const auto&) { ++completed; }};

  std::size_t expected_parties = 2;
  for (const int floor : {25, 50, 75}) {
    for (const auto biome : kOriginBiomes | std::views::reverse)
      commit_floor(account, ctx, biome, floor);
    REQUIRE(account.parties().size() == expected_parties);
    ++expected_parties;
  }
  REQUIRE(account.parties().size() == 4);
  REQUIRE(account.parties()[1].members().empty());
  REQUIRE(account.parties()[2].members().empty());
  REQUIRE(opportunities == 15);
  REQUIRE(unlocked == 3);

  std::size_t claimed = 0;
  for (const auto biome : kOriginBiomes) {
    const auto id = std::string(biome.substr(0, biome.size() - 6)) + "_floor_75_recruit";
    REQUIRE(account.claim_recruitment(ctx, id));
    ++claimed;
    REQUIRE(account.parties().size() == (claimed == 5 ? 5 : 4));
  }
  REQUIRE(account.parties()[3].members().size() == 5);
  REQUIRE(account.parties().size() == 5);
  REQUIRE(completed == 1);
  REQUIRE(unlocked == 4);
  REQUIRE(opportunities == 20);
  REQUIRE(fifth_party_batch_was_atomic);

  for (int index = 1; index <= 5; ++index)
    REQUIRE(account.claim_recruitment(ctx, "origin_party_five_recruit_" + std::to_string(index)));
  REQUIRE(account.parties()[4].members().size() == 5);
  REQUIRE(completed == 2);
  REQUIRE_FALSE(account.claim_recruitment(ctx, "origin_party_five_recruit_1"));
  commit_floor(account, ctx, "origin_desert_tower", 75);
  REQUIRE(unlocked == 4);
  REQUIRE(opportunities == 20);
}

TEST_CASE("Committed out-of-tier milestones wait for a creatable destination without being lost",
          "[recruitment][milestones][ordering]") {
  fl::GrandCentral game{1, 1, 0, true, true, 17, true};
  auto ctx = game.account_context(0);
  auto& account = ctx.account_data();
  commit_floor(account, ctx, "origin_desert_tower", 50);
  REQUIRE(account.parties().size() == 1);
  REQUIRE(opportunity(account, "origin_desert_floor_50_recruit").state ==
          fl::primitives::RecruitmentOpportunityState::Locked);

  commit_floor(account, ctx, "origin_woodland_tower", 25);
  REQUIRE(account.parties().size() == 3);
  REQUIRE(opportunity(account, "origin_woodland_floor_25_recruit").state ==
          fl::primitives::RecruitmentOpportunityState::Available);
  REQUIRE(opportunity(account, "origin_desert_floor_50_recruit").state ==
          fl::primitives::RecruitmentOpportunityState::Available);
}

TEST_CASE("Production floor-five victory unlocks and manually claims a stable recruit",
          "[regression][recruitment][integration][production]") {
  fl::GrandCentral game{1, 1, 0, true, true, 11, true, true};
  auto ctx = game.account_context(0);
  auto& account = ctx.account_data();
  auto& party = account.parties().front();

  std::vector<std::string> ordered;
  fl::events::ScopedPartyListener floor_sub{
      party.party_bus(), std::in_place_type<fl::events::TowerFloorCleared>, [&](const auto& event) {
        if (event.floor == 5) ordered.emplace_back("floor-cleared");
      }};
  fl::events::ScopedPartyListener opportunity_sub{
      party.party_bus(), std::in_place_type<fl::events::RecruitmentOpportunityUnlocked>,
      [&](const auto& event) {
        if (event.opportunity_id == "origin_woodland_floor_5_recruit")
          ordered.emplace_back("opportunity-unlocked");
      }};
  fl::events::ScopedPartyListener hero_sub{
      party.party_bus(), std::in_place_type<fl::events::HeroRecruited>, [&](const auto& event) {
        if (event.opportunity_id == "origin_woodland_floor_5_recruit")
          ordered.emplace_back("hero-recruited");
      }};

  game.objectives()->emit("visitor_destroyed_village");
  game.innervate_event_system(false);
  int beats = 0;
  while (party.highest_tower_floor_cleared("origin_woodland_tower") < 5 && beats < 30'000) {
    game.advance_beat();
    ++beats;
  }
  CAPTURE(party.highest_tower_floor_cleared("origin_woodland_tower"),
          party.latest_tower_floor("origin_woodland_tower"), party.tower_floor_attempts(),
          party.members().size(), party.has_encounter(), party.all_members_dead());
  REQUIRE(beats < 30'000);
  REQUIRE(ordered == std::vector<std::string>{"floor-cleared", "opportunity-unlocked"});
  game.objectives()->request_recruitment("origin_woodland_floor_5_recruit");
  REQUIRE(party.members().size() == 3);
  REQUIRE(party.members().back().name() == "Cyran Vale");
  REQUIRE(ordered ==
          std::vector<std::string>{"floor-cleared", "opportunity-unlocked", "hero-recruited"});
}
