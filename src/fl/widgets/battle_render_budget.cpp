#include "fl/widgets/battle_render_budget.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string>

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
constexpr int kStagePaddingWidth = 150;

constexpr std::array<BattleRenderTargetSpec, 5> kSupportedTargets{
    BattleRenderTargetSpec{BattleLayoutProfile::Tiny, kTinyWidth, kTinyHeight,
                           "Tiny"},
    BattleRenderTargetSpec{BattleLayoutProfile::Compact, kCompactWidth,
                           kCompactHeight, "Compact"},
    BattleRenderTargetSpec{BattleLayoutProfile::Standard, kStandardWidth,
                           kStandardHeight, "Standard"},
    BattleRenderTargetSpec{BattleLayoutProfile::Wide, kWideWidth, kWideHeight,
                           "Wide"},
    BattleRenderTargetSpec{BattleLayoutProfile::Showcase, kShowcaseWidth,
                           kShowcaseHeight, "Showcase"},
};

std::optional<BattleLayoutProfile> g_forced_target;

int stage_side_padding_width_for(int width) {
  return width >= kStagePaddingWidth ? std::max(2, width / 20) : 0;
}

bool reaches(int width, int height, int target_width, int target_height) {
  return width >= target_width && height >= target_height;
}

std::string normalized(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (char c : value) {
    if (c == '-' || c == '_' || std::isspace(static_cast<unsigned char>(c))) {
      continue;
    }
    out.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  return out;
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

std::span<const BattleRenderTargetSpec> supported_battle_render_targets() {
  return kSupportedTargets;
}

std::optional<BattleLayoutProfile>
battle_render_profile_from_name(std::string_view name) {
  const auto wanted = normalized(name);
  for (const auto &target : kSupportedTargets) {
    if (normalized(target.name) == wanted) {
      return target.profile;
    }
  }
  return std::nullopt;
}

BattleRenderTargetSpec battle_render_target_spec(BattleLayoutProfile profile) {
  for (const auto &target : kSupportedTargets) {
    if (target.profile == profile) {
      return target;
    }
  }
  return kSupportedTargets.front();
}

void force_battle_render_target(BattleLayoutProfile profile) {
  g_forced_target = profile;
}

void clear_forced_battle_render_target() { g_forced_target.reset(); }

std::optional<BattleLayoutProfile> forced_battle_render_target() {
  return g_forced_target;
}

BattleRenderBudget select_battle_render_budget(int width, int height) {
  BattleRenderBudget budget;
  budget.requested_width = std::max(1, width);
  budget.requested_height = std::max(1, height);
  budget.stage_side_padding_width =
      stage_side_padding_width_for(budget.requested_width);
  budget.show_auxiliary_battle_panel = budget.requested_width >= kWideWidth;

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
  if (g_forced_target) {
    const auto target = battle_render_target_spec(*g_forced_target);
    return select_battle_render_budget(target.width, target.height);
  }

  const auto terminal = ftxui::Terminal::Size();
  return select_battle_render_budget(terminal.dimx, terminal.dimy);
}

} // namespace fl::widgets
