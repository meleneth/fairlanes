#include <array>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "fl/widgets/battle_render_budget.hpp"

namespace {

using fl::widgets::BattleLayoutProfile;
using fl::widgets::CombatantDetailLevel;
using fl::widgets::PartyRenderingMode;
using fl::widgets::select_battle_render_budget;

int enabled_feature_count(const fl::widgets::BattleRenderBudget &budget) {
  return static_cast<int>(budget.show_full_combatants) +
         static_cast<int>(budget.show_party_strips) +
         static_cast<int>(budget.show_underlays) +
         static_cast<int>(budget.show_field_ambience) +
         static_cast<int>(budget.show_skill_decals) +
         static_cast<int>(budget.show_full_status_labels);
}

} // namespace

TEST_CASE("Battle render budget selects canonical target profiles",
          "[widgets][battle-budget]") {
  struct Case {
    int width;
    int height;
    BattleLayoutProfile profile;
    std::string_view name;
    std::size_t visible_parties;
    int log_lines;
  };

  const std::array cases{
      Case{80, 24, BattleLayoutProfile::Tiny, "Tiny", 1, 5},
      Case{100, 30, BattleLayoutProfile::Compact, "Compact", 2, 8},
      Case{140, 40, BattleLayoutProfile::Standard, "Standard", 3, 15},
      Case{180, 50, BattleLayoutProfile::Wide, "Wide", 5, 15},
      Case{220, 60, BattleLayoutProfile::Showcase, "Showcase", 5, 20},
  };

  for (const auto &target : cases) {
    const auto budget =
        select_battle_render_budget(target.width, target.height);
    CHECK(budget.profile == target.profile);
    CHECK(fl::widgets::profile_name(budget.profile) == target.name);
    CHECK(budget.requested_width == target.width);
    CHECK(budget.requested_height == target.height);
    CHECK(budget.visible_party_count == target.visible_parties);
    CHECK(budget.log_line_budget == target.log_lines);
  }
}

TEST_CASE("Battle render budget has deterministic boundary behavior",
          "[widgets][battle-budget]") {
  CHECK(select_battle_render_budget(99, 30).profile ==
        BattleLayoutProfile::Tiny);
  CHECK(select_battle_render_budget(100, 29).profile ==
        BattleLayoutProfile::Tiny);
  CHECK(select_battle_render_budget(100, 30).profile ==
        BattleLayoutProfile::Compact);

  CHECK(select_battle_render_budget(139, 40).profile ==
        BattleLayoutProfile::Compact);
  CHECK(select_battle_render_budget(140, 39).profile ==
        BattleLayoutProfile::Compact);
  CHECK(select_battle_render_budget(140, 40).profile ==
        BattleLayoutProfile::Standard);

  CHECK(select_battle_render_budget(179, 50).profile ==
        BattleLayoutProfile::Standard);
  CHECK(select_battle_render_budget(180, 49).profile ==
        BattleLayoutProfile::Standard);
  CHECK(select_battle_render_budget(180, 50).profile ==
        BattleLayoutProfile::Wide);

  CHECK(select_battle_render_budget(219, 60).profile ==
        BattleLayoutProfile::Wide);
  CHECK(select_battle_render_budget(220, 59).profile ==
        BattleLayoutProfile::Wide);
  CHECK(select_battle_render_budget(220, 60).profile ==
        BattleLayoutProfile::Showcase);
}

TEST_CASE("Battle render budget feature flags enrich monotonically",
          "[widgets][battle-budget]") {
  const std::array budgets{
      select_battle_render_budget(80, 24),
      select_battle_render_budget(100, 30),
      select_battle_render_budget(140, 40),
      select_battle_render_budget(180, 50),
      select_battle_render_budget(220, 60),
  };

  for (std::size_t i = 1; i < budgets.size(); ++i) {
    CHECK(budgets[i].visible_party_count >= budgets[i - 1].visible_party_count);
    CHECK(budgets[i].log_line_budget >= budgets[i - 1].log_line_budget);
    CHECK(budgets[i].status_label_budget >= budgets[i - 1].status_label_budget);
    CHECK(enabled_feature_count(budgets[i]) >=
          enabled_feature_count(budgets[i - 1]));
  }
}

TEST_CASE("Battle render budget exposes expected target capabilities",
          "[widgets][battle-budget]") {
  const auto tiny = select_battle_render_budget(80, 24);
  CHECK(tiny.combatant_detail_level == CombatantDetailLevel::Minimal);
  CHECK(tiny.party_rendering_mode == PartyRenderingMode::ActiveBattle);
  CHECK_FALSE(tiny.show_full_combatants);
  CHECK(tiny.show_party_strips);
  CHECK_FALSE(tiny.show_underlays);
  CHECK_FALSE(tiny.show_skill_decals);

  const auto compact = select_battle_render_budget(100, 30);
  CHECK(compact.combatant_detail_level == CombatantDetailLevel::Compact);
  CHECK(compact.party_rendering_mode == PartyRenderingMode::PartyStrips);
  CHECK(compact.show_party_strips);
  CHECK_FALSE(compact.show_full_status_labels);

  const auto standard = select_battle_render_budget(140, 40);
  CHECK(standard.combatant_detail_level == CombatantDetailLevel::Full);
  CHECK(standard.party_rendering_mode == PartyRenderingMode::FullParties);
  CHECK(standard.show_full_combatants);
  CHECK(standard.show_underlays);
  CHECK(standard.show_skill_decals);
  CHECK_FALSE(standard.show_field_ambience);

  const auto showcase = select_battle_render_budget(220, 60);
  CHECK(showcase.combatant_detail_level == CombatantDetailLevel::Showcase);
  CHECK(showcase.party_rendering_mode == PartyRenderingMode::FullParties);
  CHECK(showcase.show_full_combatants);
  CHECK(showcase.show_underlays);
  CHECK(showcase.show_field_ambience);
  CHECK(showcase.show_skill_decals);
  CHECK(showcase.show_full_status_labels);
}

TEST_CASE("Battle render budget preserves requested dimensions defensively",
          "[widgets][battle-budget]") {
  const auto tiny_terminal = select_battle_render_budget(1, 1);
  CHECK(tiny_terminal.profile == BattleLayoutProfile::Tiny);
  CHECK(tiny_terminal.requested_width == 1);
  CHECK(tiny_terminal.requested_height == 1);

  const auto invalid_terminal = select_battle_render_budget(-20, 0);
  CHECK(invalid_terminal.profile == BattleLayoutProfile::Tiny);
  CHECK(invalid_terminal.requested_width == 1);
  CHECK(invalid_terminal.requested_height == 1);
}
