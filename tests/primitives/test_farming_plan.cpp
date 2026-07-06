#include <catch2/catch_test_macros.hpp>

#include <array>
#include <set>

#include "fl/fsm/party_loop.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/farming_plan.hpp"
#include "fl/widgets/farming_choice_view.hpp"

TEST_CASE("Farm focus definitions include the five focused choices",
          "[farming][plan]") {
  const auto definitions = fl::primitives::all_farm_focus_definitions();
  REQUIRE(definitions.size() == 5);

  std::set<fl::primitives::FarmFocus> focuses;
  for (const auto &definition : definitions) {
    focuses.insert(definition.focus);
    REQUIRE_FALSE(definition.display_name.empty());
    REQUIRE_FALSE(definition.description.empty());
  }

  REQUIRE(focuses.contains(fl::primitives::FarmFocus::Brawn));
  REQUIRE(focuses.contains(fl::primitives::FarmFocus::Cunning));
  REQUIRE(focuses.contains(fl::primitives::FarmFocus::Wisdom));
  REQUIRE(focuses.contains(fl::primitives::FarmFocus::WealthMaterials));
  REQUIRE(focuses.contains(fl::primitives::FarmFocus::Gear));
}

TEST_CASE("Discipline farm classification distinguishes aligned and cross",
          "[farming][plan]") {
  using fl::primitives::FarmFocus;
  using fl::primitives::FarmRewardClass;
  using fl::primitives::GrimoireDiscipline;

  REQUIRE(fl::primitives::classify_farming_plan(GrimoireDiscipline::Brawn,
                                                FarmFocus::Brawn) ==
          FarmRewardClass::AlignedDiscipline);
  REQUIRE(fl::primitives::classify_farming_plan(GrimoireDiscipline::Brawn,
                                                FarmFocus::Cunning) ==
          FarmRewardClass::CrossDiscipline);
  REQUIRE(fl::primitives::classify_farming_plan(GrimoireDiscipline::Wisdom,
                                                FarmFocus::Brawn) ==
          FarmRewardClass::CrossDiscipline);
}

TEST_CASE("Wealth material and gear farms classify away from discipline growth",
          "[farming][plan]") {
  using fl::primitives::FarmFocus;
  using fl::primitives::FarmRewardClass;
  using fl::primitives::GrimoireDiscipline;

  REQUIRE(fl::primitives::classify_farming_plan(GrimoireDiscipline::Cunning,
                                                FarmFocus::WealthMaterials) ==
          FarmRewardClass::WealthMaterials);
  REQUIRE(fl::primitives::classify_farming_plan(GrimoireDiscipline::Wisdom,
                                                FarmFocus::Gear) ==
          FarmRewardClass::Gear);
}

TEST_CASE("Grimoire discipline and farm focus construct narrow reward plans",
          "[farming][plan]") {
  using fl::primitives::FarmFocus;
  using fl::primitives::GrimoireDiscipline;

  const auto aligned = fl::primitives::make_farming_plan(
      GrimoireDiscipline::Cunning, FarmFocus::Cunning);
  REQUIRE(fl::primitives::plan_is_aligned(aligned));
  REQUIRE(aligned.grimoire_progress_weight == 100);
  REQUIRE(aligned.breakthrough_progress_weight == 0);

  const auto cross = fl::primitives::make_farming_plan(
      GrimoireDiscipline::Cunning, FarmFocus::Wisdom);
  REQUIRE(fl::primitives::plan_is_cross_discipline(cross));
  REQUIRE(cross.grimoire_progress_weight < aligned.grimoire_progress_weight);
  REQUIRE(cross.breakthrough_progress_weight > 0);

  const auto wealth = fl::primitives::make_farming_plan(
      GrimoireDiscipline::Wisdom, FarmFocus::WealthMaterials);
  REQUIRE(wealth.economy_progress_weight > wealth.grimoire_progress_weight);
  REQUIRE(wealth.breakthrough_progress_weight == 0);

  const auto gear = fl::primitives::make_farming_plan(GrimoireDiscipline::Brawn,
                                                      FarmFocus::Gear);
  REQUIRE(gear.gear_progress_weight > gear.grimoire_progress_weight);
  REQUIRE(gear.breakthrough_progress_weight == 0);
}

TEST_CASE("Farming choice is applied once after grimoire discipline selection",
          "[farming][start-flow]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();

  party.select_grimoire_discipline(fl::primitives::GrimoireDiscipline::Wisdom);
  REQUIRE(party.needs_farm_focus_choice());

  fl::fsm::PartyLoop::Ops::enter_farming(party_ctx);
  REQUIRE_FALSE(party.has_encounter());

  fl::widgets::FarmingChoiceView choice{
      party, fl::primitives::GrimoireDiscipline::Wisdom};
  REQUIRE(choice.OnEvent(ftxui::Event::Character("2")));
  REQUIRE_FALSE(party.needs_farm_focus_choice());
  REQUIRE(party.farming_plan().discipline ==
          fl::primitives::GrimoireDiscipline::Wisdom);
  REQUIRE(party.farming_plan().focus == fl::primitives::FarmFocus::Cunning);
  REQUIRE(party.farming_plan().reward_class ==
          fl::primitives::FarmRewardClass::CrossDiscipline);

  REQUIRE_FALSE(choice.OnEvent(ftxui::Event::Character("5")));
  REQUIRE(party.farming_plan().focus == fl::primitives::FarmFocus::Cunning);

  fl::fsm::PartyLoop::Ops::enter_farming(party_ctx);
  REQUIRE(party.has_encounter());
}
