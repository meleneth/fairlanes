#include "fl/widgets/battle_render_budget.hpp"

#include <algorithm>

#include <ftxui/screen/terminal.hpp>

namespace fl::widgets {
namespace {

constexpr int kTinyWidth = 80;
constexpr int kTinyHeight = 24;
constexpr int kCompactWidth = 100;
constexpr int kCompactHeight = 30;
constexpr int kStandardWidth = 140;
constexpr int kStandardHeight = 40;
constexpr int kWideWidth = 180;
constexpr int kWideHeight = 50;
constexpr int kShowcaseWidth = 220;
constexpr int kShowcaseHeight = 60;

bool reaches(int width, int height, int target_width, int target_height) {
  return width >= target_width && height >= target_height;
}

} // namespace

std::string_view profile_name(BattleLayoutProfile profile) {
  switch (profile) {
  case BattleLayoutProfile::Tiny:
    return "Tiny";
  case BattleLayoutProfile::Compact:
    return "Compact";
  case BattleLayoutProfile::Standard:
    return "Standard";
  case BattleLayoutProfile::Wide:
    return "Wide";
  case BattleLayoutProfile::Showcase:
    return "Showcase";
  }

  return "Tiny";
}

BattleRenderBudget select_battle_render_budget(int width, int height) {
  BattleRenderBudget budget;
  budget.requested_width = std::max(1, width);
  budget.requested_height = std::max(1, height);

  if (reaches(budget.requested_width, budget.requested_height, kShowcaseWidth,
              kShowcaseHeight)) {
    budget.profile = BattleLayoutProfile::Showcase;
    budget.combatant_detail_level = CombatantDetailLevel::Showcase;
    budget.party_rendering_mode = PartyRenderingMode::FullParties;
    budget.visible_party_count = 5;
    budget.log_line_budget = 20;
    budget.status_label_budget = 6;
    budget.show_full_combatants = true;
    budget.show_party_strips = true;
    budget.show_underlays = true;
    budget.show_field_ambience = true;
    budget.show_skill_decals = true;
    budget.show_full_status_labels = true;
    return budget;
  }

  if (reaches(budget.requested_width, budget.requested_height, kWideWidth,
              kWideHeight)) {
    budget.profile = BattleLayoutProfile::Wide;
    budget.combatant_detail_level = CombatantDetailLevel::Full;
    budget.party_rendering_mode = PartyRenderingMode::FullParties;
    budget.visible_party_count = 5;
    budget.log_line_budget = 15;
    budget.status_label_budget = 5;
    budget.show_full_combatants = true;
    budget.show_party_strips = true;
    budget.show_underlays = true;
    budget.show_field_ambience = true;
    budget.show_skill_decals = true;
    budget.show_full_status_labels = true;
    return budget;
  }

  if (reaches(budget.requested_width, budget.requested_height, kStandardWidth,
              kStandardHeight)) {
    budget.profile = BattleLayoutProfile::Standard;
    budget.combatant_detail_level = CombatantDetailLevel::Full;
    budget.party_rendering_mode = PartyRenderingMode::FullParties;
    budget.visible_party_count = 3;
    budget.log_line_budget = 15;
    budget.status_label_budget = 4;
    budget.show_full_combatants = true;
    budget.show_party_strips = true;
    budget.show_underlays = true;
    budget.show_field_ambience = false;
    budget.show_skill_decals = true;
    budget.show_full_status_labels = true;
    return budget;
  }

  if (reaches(budget.requested_width, budget.requested_height, kCompactWidth,
              kCompactHeight)) {
    budget.profile = BattleLayoutProfile::Compact;
    budget.combatant_detail_level = CombatantDetailLevel::Compact;
    budget.party_rendering_mode = PartyRenderingMode::PartyStrips;
    budget.visible_party_count = 2;
    budget.log_line_budget = 8;
    budget.status_label_budget = 2;
    budget.show_full_combatants = false;
    budget.show_party_strips = true;
    budget.show_underlays = false;
    budget.show_field_ambience = false;
    budget.show_skill_decals = false;
    budget.show_full_status_labels = false;
    return budget;
  }

  budget.profile = BattleLayoutProfile::Tiny;
  budget.combatant_detail_level = CombatantDetailLevel::Minimal;
  budget.party_rendering_mode = PartyRenderingMode::ActiveBattle;
  budget.visible_party_count = 1;
  budget.log_line_budget = 5;
  budget.status_label_budget = 1;
  budget.show_full_combatants = false;
  budget.show_party_strips = true;
  budget.show_underlays = false;
  budget.show_field_ambience = false;
  budget.show_skill_decals = false;
  budget.show_full_status_labels = false;
  return budget;
}

BattleRenderBudget current_battle_render_budget() {
  const auto terminal = ftxui::Terminal::Size();
  return select_battle_render_budget(terminal.dimx, terminal.dimy);
}

} // namespace fl::widgets
