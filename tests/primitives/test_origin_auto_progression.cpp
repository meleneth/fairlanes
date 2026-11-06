#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

#include "fl/ecs/components/stats.hpp"
#include "fl/events/story_bus.hpp"
#include "fl/generated/story_content.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/objective_system.hpp"
#include "fl/primitives/origin_auto_progression.hpp"

namespace {

using Decision = std::tuple<std::string, std::string, int>;

struct AutoHarness {
  fl::GrandCentral game{1, 1, 1, true, true, 7};
  fl::context::AccountCtx account{game.account_context(0)};
  fl::primitives::ObjectiveSystem story{account};
  fl::primitives::OriginAutoProgression controller{account, 0, story.bus()};
  std::vector<Decision> decisions;
  fl::events::ScopedStoryListener decision_sub{
      story.bus(), std::in_place_type<fl::events::OriginAutoProgressionDecision>,
      [this](const auto& event) {
        decisions.emplace_back(event.action, event.target, event.floor);
      }};

  AutoHarness() {
    account.account_data().enable_origin_recruitment();
  }

  fl::primitives::PartyData& party() {
    return account.account_data().party(0);
  }

  fl::context::PartyCtx party_ctx() {
    return account.party_context(0);
  }

  void enable() {
    story.request_autoplay(true);
    controller.evaluate();
  }

  void clear_selected_floor() {
    auto ctx = party_ctx();
    const auto route = *party().tower_route();
    party().begin_tower_encounter();
    auto& encounter = fl::primitives::EncounterBuilder{
        ctx, party().encounter_mode(),
        fl::primitives::EncounterBuilder::TowerFloor{
            std::string(route.biome_id),
            route.floor}}.thump_it_out();
    for (const auto enemy : encounter.attackers().members())
      ctx.reg().get<fl::ecs::components::Stats>(enemy).hp_ = 0;
    party().leave_combat();
    account.account_data().resolve_recruitment_events(account);
    controller.evaluate();
  }

  void wipe_selected_floor() {
    auto ctx = party_ctx();
    const auto route = *party().tower_route();
    party().begin_tower_encounter();
    fl::primitives::EncounterBuilder{
        ctx, party().encounter_mode(),
        fl::primitives::EncounterBuilder::TowerFloor{std::string(route.biome_id), route.floor}}
        .thump_it_out();
    for (const auto& member : party().members())
      ctx.reg().get<fl::ecs::components::Stats>(member.member_id()).hp_ = 0;
    party().party_bus().emit(fl::events::PartyEvent{fl::events::PartyWiped{}});
    party().revitalize_members();
    controller.evaluate();
  }
};

} // namespace

TEST_CASE("Origin autoplay selects a legal route and stops strategic decisions in manual mode",
          "[origin-autoplay][mode]") {
  AutoHarness harness;
  harness.enable();
  REQUIRE(harness.controller.enabled());
  REQUIRE(harness.party().tower_route()->biome_id == "origin_woodland_tower");
  REQUIRE(harness.party().progression_control_mode() ==
          fl::primitives::ProgressionControlMode::Auto);

  auto ctx = harness.party_ctx();
  const auto route = *harness.party().tower_route();
  harness.party().begin_tower_encounter();
  auto& encounter = fl::primitives::EncounterBuilder{
      ctx, harness.party().encounter_mode(),
      fl::primitives::EncounterBuilder::TowerFloor{
          std::string(route.biome_id),
          route.floor}}.thump_it_out();
  harness.story.request_autoplay(false);
  const auto decisions_after_disable = harness.decisions.size();
  harness.controller.evaluate();
  REQUIRE_FALSE(harness.controller.enabled());
  REQUIRE(harness.decisions.size() == decisions_after_disable);
  REQUIRE(harness.party().progression_control_mode() ==
          fl::primitives::ProgressionControlMode::Manual);

  harness.story.request_autoplay(true);
  const auto decisions_during_combat = harness.decisions.size();
  harness.controller.evaluate();
  REQUIRE(harness.controller.enabled());
  REQUIRE(harness.decisions.size() == decisions_during_combat);
  for (const auto enemy : encounter.attackers().members())
    ctx.reg().get<fl::ecs::components::Stats>(enemy).hp_ = 0;
  harness.party().leave_combat();
  harness.controller.evaluate();
  REQUIRE(harness.decisions.size() > decisions_during_combat);
}

TEST_CASE("Production autoplay continues from the first clear into the next encounter",
          "[origin-autoplay][production][continuation]") {
  fl::GrandCentral game{1, 1, 0, true, true, 7, true, true};
  auto& party = game.accounts().front().party(0);
  game.innervate_event_system();
  int beats = 0;
  while ((party.highest_tower_floor_cleared("origin_woodland_tower") < 1 || !party.in_combat() ||
          party.tower_route()->floor != 2) &&
         beats < 5000) {
    game.advance_beat();
    ++beats;
  }
  REQUIRE(party.highest_tower_floor_cleared("origin_woodland_tower") == 1);
  REQUIRE(party.tower_route()->floor == 2);
  REQUIRE(party.in_combat());
}

TEST_CASE("Origin autoplay follows frontier clears without treating replay as another clear",
          "[origin-autoplay][progression]") {
  AutoHarness harness;
  harness.enable();
  harness.clear_selected_floor();
  REQUIRE(harness.party().highest_tower_floor_cleared("origin_woodland_tower") == 1);
  REQUIRE(harness.party().tower_route()->floor == 2);

  REQUIRE(harness.party().select_tower("origin_woodland_tower", 1));
  harness.clear_selected_floor();
  REQUIRE(harness.party().highest_tower_floor_cleared("origin_woodland_tower") == 1);
  REQUIRE(harness.party().tower_route()->floor == 2);
}

TEST_CASE("Origin autoplay traverses the established five-biome unlock order",
          "[origin-autoplay][progression][biomes]") {
  AutoHarness harness;
  harness.enable();

  for (std::size_t biome_index = 0;
       biome_index + 1 < fl::primitives::PartyData::kOriginBiomeIds.size(); ++biome_index) {
    const auto biome = fl::primitives::PartyData::kOriginBiomeIds[biome_index];
    while (harness.party().highest_tower_floor_cleared(biome) < 9) harness.clear_selected_floor();
    REQUIRE(harness.party().tower_route()->biome_id ==
            fl::primitives::PartyData::kOriginBiomeIds[biome_index + 1]);
    REQUIRE(harness.party().tower_route()->floor == 1);
  }
}

TEST_CASE("Origin autoplay stops at the completed Desert tower instead of constructing floor 101",
          "[origin-autoplay][progression][completion]") {
  AutoHarness harness;
  harness.enable();
  while (harness.party().tower_route()->biome_id != "origin_desert_tower" ||
         harness.party().highest_tower_floor_cleared("origin_desert_tower") < 100)
    harness.clear_selected_floor();

  REQUIRE(harness.party().tower_route()->biome_id == "origin_desert_tower");
  REQUIRE(harness.party().tower_route()->floor == 100);
  REQUIRE(harness.party().encounter_start_blocked());
  REQUIRE(std::get<0>(harness.decisions.back()) == "blocked");
  REQUIRE(std::get<1>(harness.decisions.back()) == "Origin route complete");
}

TEST_CASE("Origin autoplay claims legal recruitment and leaves surplus opportunities alone",
          "[origin-autoplay][recruitment]") {
  AutoHarness harness;
  harness.enable();
  while (harness.party().highest_tower_floor_cleared("origin_woodland_tower") < 5)
    harness.clear_selected_floor();
  harness.controller.evaluate();
  REQUIRE(harness.party().members().size() == 2);

  while (harness.party().members().size() < 5)
    harness.party().recruit_origin_member("Capacity Tester");
  const auto recruit_decisions = std::ranges::count_if(
      harness.decisions, [](const auto& entry) { return std::get<0>(entry) == "recruit"; });
  harness.party().party_bus().emit(fl::events::PartyEvent{
      fl::events::RecruitmentOpportunityUnlocked{"unavailable", "origin_woodland_tower", 5, 0}});
  harness.controller.evaluate();
  harness.controller.evaluate();
  REQUIRE(std::ranges::count_if(harness.decisions, [](const auto& entry) {
            return std::get<0>(entry) == "recruit";
          }) == recruit_decisions);
}

TEST_CASE("Origin autoplay bounds retries, farms, and returns to the failed frontier",
          "[origin-autoplay][recovery]") {
  AutoHarness harness;
  harness.enable();
  harness.clear_selected_floor();
  REQUIRE(harness.party().tower_route()->floor == 2);

  harness.wipe_selected_floor();
  harness.wipe_selected_floor();
  REQUIRE(harness.party().tower_route()->floor == 2);
  harness.wipe_selected_floor();
  REQUIRE(harness.party().tower_route()->floor == 1);

  harness.clear_selected_floor();
  REQUIRE(harness.party().tower_route()->floor == 1);
  harness.clear_selected_floor();
  REQUIRE(harness.party().tower_route()->floor == 2);
  harness.clear_selected_floor();
  REQUIRE(harness.party().tower_route()->floor == 3);
}

TEST_CASE("Origin autoplay blocks after bounded floor-one failures and manual mode releases it",
          "[origin-autoplay][recovery][blocked]") {
  AutoHarness harness;
  harness.enable();
  harness.wipe_selected_floor();
  harness.wipe_selected_floor();
  harness.wipe_selected_floor();
  REQUIRE(harness.party().encounter_start_blocked());
  REQUIRE(std::get<0>(harness.decisions.back()) == "blocked");

  harness.story.request_autoplay(false);
  REQUIRE_FALSE(harness.party().encounter_start_blocked());
}

TEST_CASE("Origin autoplay decisions replay deterministically",
          "[origin-autoplay][regression][deterministic]") {
  REQUIRE(fl::story::generated_content::kOriginAutoRegressions.size() == 1);
  const auto& regression = fl::story::generated_content::kOriginAutoRegressions.front();
  REQUIRE(regression.seed == 7);
  const auto run = [&regression] {
    AutoHarness harness;
    harness.enable();
    for (int clear = 0; clear < regression.clears; ++clear) harness.clear_selected_floor();
    return harness.decisions;
  };
  const auto first = run();
  const auto second = run();
  REQUIRE(first == second);
  std::vector<Decision> expected;
  for (const auto& decision : regression.expected_decisions)
    expected.emplace_back(decision.action, decision.target, decision.floor);
  REQUIRE(first == expected);
}
