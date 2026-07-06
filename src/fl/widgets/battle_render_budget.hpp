#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace fl::widgets {

enum class BattleLayoutProfile {
  Tiny,
  Compact,
  Standard,
  Wide,
  Showcase,
};

enum class CombatantDetailLevel {
  Minimal,
  Strip,
  Compact,
  Full,
  Showcase,
};

enum class PartyRenderingMode {
  ActiveBattle,
  PartyStrips,
  FullParties,
};

struct BattleRenderTargetSpec {
  BattleLayoutProfile profile{BattleLayoutProfile::Tiny};
  int width{80};
  int height{24};
  std::string_view name{"Tiny"};
};

struct BattleRenderBudget {
  BattleLayoutProfile profile{BattleLayoutProfile::Tiny};
  int requested_width{80};
  int requested_height{24};
  CombatantDetailLevel combatant_detail_level{CombatantDetailLevel::Minimal};
  PartyRenderingMode party_rendering_mode{PartyRenderingMode::ActiveBattle};
  std::size_t visible_party_count{1};
  int log_line_budget{5};
  int status_label_budget{1};
  int stage_side_padding_width{0};
  bool show_full_combatants{false};
  bool show_party_strips{false};
  bool show_underlays{false};
  bool show_field_ambience{false};
  bool show_skill_decals{false};
  bool show_full_status_labels{false};
  bool show_auxiliary_battle_panel{false};
};

std::string_view profile_name(BattleLayoutProfile profile);
std::span<const BattleRenderTargetSpec> supported_battle_render_targets();
std::optional<BattleLayoutProfile>
battle_render_profile_from_name(std::string_view name);
BattleRenderTargetSpec battle_render_target_spec(BattleLayoutProfile profile);
void force_battle_render_target(BattleLayoutProfile profile);
void clear_forced_battle_render_target();
std::optional<BattleLayoutProfile> forced_battle_render_target();
BattleRenderBudget select_battle_render_budget(int width, int height);
BattleRenderBudget current_battle_render_budget();

} // namespace fl::widgets
