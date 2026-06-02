#include "party_battle_screen.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "fl/ecs/components/is_account.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/combatant.hpp"
#include "fl/widgets/party_status.hpp"
#include "fl/widgets/textures/bog_background.hpp"
#include "fl/widgets/textures/forest_background.hpp"
#include "fl/widgets/textures/savannah_background.hpp"

namespace fl::widgets {
namespace {

constexpr std::size_t kColumns = 3;
constexpr std::size_t kRows = 2;
constexpr std::size_t kMaxCombatants = kColumns * kRows;
constexpr int kRootChromeHeight = 3;
constexpr int kMinCellWidth = 28;
constexpr int kMinCellHeight = 5;
constexpr int kStatusHeight = 4;
constexpr int kSeparatorHeight = 1;
constexpr int kMinBottomPanelHeight = 4;
constexpr std::size_t kStageBackgroundCount = 3;

struct Layout {
  int screen_width{1};
  int screen_height{1};
  int status_height{kStatusHeight};
  int separator_height{kSeparatorHeight};
  int bottom_height{kMinBottomPanelHeight};
  int combatant_cell_width{kMinCellWidth};
  int combatant_cell_height{kMinCellHeight};
  int combatant_grid_width{kMinCellWidth * static_cast<int>(kColumns)};
  int combatant_grid_height{kMinCellHeight * static_cast<int>(kRows)};
  int stage_padding_width{0};
};

Layout current_layout(const bool show_status) {
  const auto terminal = ftxui::Terminal::Size();
  Layout layout;
  layout.status_height = show_status ? kStatusHeight : 0;

  layout.screen_width = std::max(1, terminal.dimx);
  const int active_height = std::max(1, terminal.dimy - kRootChromeHeight);

  const int fixed_height = layout.status_height + (layout.separator_height * 2);
  const int variable_height = std::max(1, active_height - fixed_height);
  const int target_bottom_height = std::min(
      variable_height, std::max(kMinBottomPanelHeight, active_height / 4));
  const int target_combat_height =
      std::max(1, variable_height - target_bottom_height);

  layout.combatant_cell_height = std::max(1, target_combat_height / 4);
  if (target_combat_height >= kMinCellHeight * 4) {
    layout.combatant_cell_height =
        std::max(kMinCellHeight, layout.combatant_cell_height);
  }
  layout.combatant_grid_height =
      layout.combatant_cell_height * static_cast<int>(kRows);
  layout.bottom_height = std::max(1, active_height - fixed_height -
                                         (layout.combatant_grid_height * 2));

  const int preferred_padding =
      layout.screen_width >= 150 ? std::max(2, layout.screen_width / 20) : 0;
  const int available_stage_width =
      std::max(kMinCellWidth * static_cast<int>(kColumns),
               layout.screen_width - (preferred_padding * 2));
  layout.combatant_cell_width = std::max(
      kMinCellWidth, available_stage_width / static_cast<int>(kColumns));
  layout.combatant_grid_width =
      layout.combatant_cell_width * static_cast<int>(kColumns);
  layout.stage_padding_width =
      std::max(0, (layout.screen_width - layout.combatant_grid_width) / 2);

  layout.screen_height = layout.status_height +
                         (layout.combatant_grid_height * 2) +
                         (layout.separator_height * 2) + layout.bottom_height;
  return layout;
}

ftxui::Element labeled_separator(std::string label, int width) {
  using namespace ftxui;

  if (label.empty()) {
    label = "Party";
  }

  const std::string marker = "-- " + label + " ";
  const int fill = std::max(0, width - static_cast<int>(marker.size()));
  return text(marker + std::string(static_cast<std::size_t>(fill), '-')) |
         size(WIDTH, EQUAL, width);
}

std::uint32_t forest_seed_for_party(const std::string &party_name,
                                    std::size_t party_index) {
  std::uint32_t seed = 0x46A7B055u ^ static_cast<std::uint32_t>(
                                         (party_index + 1U) * 0x9E3779B9u);
  for (unsigned char ch : party_name) {
    seed ^= static_cast<std::uint32_t>(ch);
    seed *= 0x85EBCA6Bu;
    seed ^= seed >> 13;
  }
  return seed;
}

std::vector<entt::entity>
member_entities(const std::deque<fl::primitives::MemberData> &members) {
  std::vector<entt::entity> entities;
  entities.reserve(std::min(members.size(), kMaxCombatants));

  for (const auto &member : members) {
    if (entities.size() >= kMaxCombatants) {
      break;
    }
    entities.push_back(member.member_id());
  }

  return entities;
}

} // namespace

PartyBattleScreen::PartyBattleScreen(fl::context::AccountCtx ctx,
                                     std::size_t party_index)
    : ctx_(std::move(ctx)), party_index_(party_index) {}

bool PartyBattleScreen::OnEvent(ftxui::Event event) {
  auto *check_is_account =
      ctx_.reg().try_get<fl::ecs::components::IsAccount>(ctx_.self());
  if (!check_is_account) {
    return false;
  }

  auto &account = check_is_account->account_data();
  const bool has_party_log = !account.parties().empty();
  const int log_count = has_party_log ? 2 : 1;
  focused_log_ = std::clamp(focused_log_, 0, log_count - 1);

  if (event == ftxui::Event::Tab) {
    focused_log_ = (focused_log_ + 1) % log_count;
    return true;
  }

  if (event == ftxui::Event::TabReverse) {
    focused_log_ = (focused_log_ + log_count - 1) % log_count;
    return true;
  }

  if (focused_log_ == 0) {
    return account.log().OnEvent(event);
  }

  auto &parties = account.parties();
  if (parties.empty()) {
    return false;
  }

  const auto selected = std::min(party_index_, parties.size() - 1);
  return parties[selected].log().OnEvent(event);
}

ftxui::Element PartyBattleScreen::Render() {
  using namespace ftxui;

  auto *check_is_account =
      ctx_.reg().try_get<fl::ecs::components::IsAccount>(ctx_.self());
  if (!check_is_account) {
    return window(text("PartyBattleScreen: ctx_.self() is not an "
                       "account"),
                  text("Missing IsAccount on ctx_.self()")) |
           bgcolor(fl::lospec500::color_at(0)) |
           color(fl::lospec500::color_at(32));
  }

  auto &account = check_is_account->account_data();
  auto &parties = account.parties();
  focused_log_ = std::clamp(focused_log_, 0, parties.empty() ? 0 : 1);

  std::vector<entt::entity> attackers;
  std::vector<entt::entity> defenders;
  fl::primitives::PartyData *primary_party = nullptr;
  if (!parties.empty()) {
    const auto selected = std::min(party_index_, parties.size() - 1);
    primary_party = &parties[selected];
  }

  const fl::primitives::EncounterData *active_encounter = nullptr;
  if (primary_party != nullptr && primary_party->has_encounter()) {
    auto &encounter = primary_party->encounter_data();
    active_encounter = &encounter;
    attackers = encounter.attackers().members();
    defenders = encounter.defenders().members();
  } else if (primary_party != nullptr) {
    defenders = member_entities(primary_party->members());
  }
  update_stage_background(active_encounter);

  const bool show_status =
      primary_party == nullptr || !primary_party->has_encounter();
  Element status = text("No parties.") | bold;
  std::string party_name = "Party";
  if (primary_party != nullptr) {
    party_name = std::string(primary_party->name());
    if (show_status) {
      status = PartyStatus{*primary_party}.Render();
    }
  }

  const auto layout = current_layout(show_status);
  const auto forest_seed = forest_seed_for_party(party_name, party_index_);

  auto attacker_stage =
      hbox({
          filler() | size(WIDTH, EQUAL, layout.stage_padding_width),
          render_combatant_grid(attackers, layout.combatant_cell_width,
                                layout.combatant_cell_height,
                                layout.combatant_grid_width,
                                layout.combatant_grid_height),
          filler() | size(WIDTH, EQUAL, layout.stage_padding_width),
      }) |
      size(WIDTH, EQUAL, layout.screen_width) |
      size(HEIGHT, EQUAL, layout.combatant_grid_height);
  auto defender_stage =
      hbox({
          filler() | size(WIDTH, EQUAL, layout.stage_padding_width),
          render_combatant_grid(defenders, layout.combatant_cell_width,
                                layout.combatant_cell_height,
                                layout.combatant_grid_width,
                                layout.combatant_grid_height),
          filler() | size(WIDTH, EQUAL, layout.stage_padding_width),
      }) |
      size(WIDTH, EQUAL, layout.screen_width) |
      size(HEIGHT, EQUAL, layout.combatant_grid_height);

  std::vector<Element> rows;
  rows.reserve(show_status ? 6 : 5);
  if (show_status) {
    rows.push_back(status | size(WIDTH, EQUAL, layout.screen_width) |
                   size(HEIGHT, EQUAL, layout.status_height));
  }
  rows.push_back(
      render_stage_background(std::move(attacker_stage), layout.screen_width,
                              layout.combatant_grid_height, forest_seed));
  rows.push_back(labeled_separator(party_name, layout.screen_width) |
                 size(HEIGHT, EQUAL, layout.separator_height) |
                 fl::lospec500::on_not_black(fl::lospec500::color_at(32)));
  rows.push_back(render_stage_background(
      std::move(defender_stage), layout.screen_width,
      layout.combatant_grid_height, forest_seed ^ 0xA53C9E2Bu));
  rows.push_back(separator() | size(WIDTH, EQUAL, layout.screen_width) |
                 size(HEIGHT, EQUAL, layout.separator_height) |
                 fl::lospec500::on_not_black(fl::lospec500::color_at(32)));
  rows.push_back(
      render_bottom_panel(layout.screen_width, layout.bottom_height));

  return vbox(std::move(rows)) | size(WIDTH, EQUAL, layout.screen_width) |
         size(HEIGHT, EQUAL, layout.screen_height) |
         bgcolor(fl::lospec500::color_at(0)) |
         color(fl::lospec500::color_at(32));
}

void PartyBattleScreen::update_stage_background(
    const fl::primitives::EncounterData *encounter) {
  if (encounter == nullptr) {
    last_encounter_ = nullptr;
    return;
  }

  if (encounter == last_encounter_) {
    return;
  }

  last_encounter_ = encounter;
  stage_background_index_ = next_background_index_;
  next_background_index_ = (next_background_index_ + 1) % kStageBackgroundCount;
}

ftxui::Element
PartyBattleScreen::render_stage_background(ftxui::Element foreground, int width,
                                           int height,
                                           std::uint32_t seed) const {
  if (stage_background_index_ == 1) {
    return textures::SavannahPanel(std::move(foreground), width, height, seed);
  }
  if (stage_background_index_ == 2) {
    return textures::BogPanel(std::move(foreground), width, height, seed);
  }

  return textures::ForestPanel(std::move(foreground), width, height, seed);
}

ftxui::Element PartyBattleScreen::render_combatant_cell(entt::entity entity,
                                                        int width, int height) {
  using namespace ftxui;

  Element content = filler();
  if (entity != entt::null && ctx_.reg().valid(entity)) {
    content = Combatant{ctx_.reg(), entity, true}.Render() |
              bgcolor(fl::lospec500::color_at(0)) | clear_under;
  }

  return content | size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height);
}

ftxui::Element PartyBattleScreen::render_combatant_grid(
    const std::vector<entt::entity> &entities, int cell_width, int cell_height,
    int grid_width, int grid_height) {
  using namespace ftxui;

  Elements rows;
  rows.reserve(kRows);
  for (std::size_t row = 0; row < kRows; ++row) {
    const std::size_t row_begin = row * kColumns;
    const std::size_t remaining =
        entities.size() > row_begin ? entities.size() - row_begin : 0;
    const std::size_t row_count = std::min(kColumns, remaining);

    Elements row_cells;
    if (row_count == 0) {
      row_cells.reserve(kColumns);
      for (std::size_t col = 0; col < kColumns; ++col) {
        row_cells.push_back(
            render_combatant_cell(entt::null, cell_width, cell_height));
      }
    } else if (row_count == 1) {
      row_cells.reserve(3);
      row_cells.push_back(filler() | size(WIDTH, EQUAL, cell_width));
      row_cells.push_back(
          render_combatant_cell(entities[row_begin], cell_width, cell_height));
      row_cells.push_back(filler() | size(WIDTH, EQUAL, cell_width));
    } else if (row_count == 2) {
      const int left_pad = std::max(0, cell_width / 2);
      const int right_pad = std::max(0, cell_width - left_pad);
      row_cells.reserve(4);
      row_cells.push_back(filler() | size(WIDTH, EQUAL, left_pad));
      row_cells.push_back(
          render_combatant_cell(entities[row_begin], cell_width, cell_height));
      row_cells.push_back(render_combatant_cell(entities[row_begin + 1],
                                                cell_width, cell_height));
      row_cells.push_back(filler() | size(WIDTH, EQUAL, right_pad));
    } else {
      row_cells.reserve(kColumns);
      for (std::size_t col = 0; col < kColumns; ++col) {
        row_cells.push_back(render_combatant_cell(entities[row_begin + col],
                                                  cell_width, cell_height));
      }
    }

    rows.push_back(hbox(std::move(row_cells)) | size(WIDTH, EQUAL, grid_width) |
                   size(HEIGHT, EQUAL, cell_height));
  }

  return vbox(std::move(rows)) | size(WIDTH, EQUAL, grid_width) |
         size(HEIGHT, EQUAL, grid_height);
}

ftxui::Element PartyBattleScreen::render_bottom_panel(int screen_width,
                                                      int height) {
  using namespace ftxui;

  const int account_width = std::max(1, screen_width / 3);
  const int party_width = std::max(1, screen_width / 3);
  const int question_width =
      std::max(1, screen_width - account_width - party_width);

  return hbox({
             render_account_log_panel(account_width, height),
             render_party_log_panel(party_width, height),
             render_question_panel(question_width, height),
         }) |
         size(WIDTH, EQUAL, screen_width) | size(HEIGHT, EQUAL, height);
}

ftxui::Element PartyBattleScreen::render_account_log_panel(int width,
                                                           int height) {
  using namespace ftxui;

  auto &account = ctx_.account_data();
  account.log().set_focused(focused_log_ == 0);

  return window(text("Account log") | bold,
                account.log().Render() | yframe | vscroll_indicator) |
         size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height);
}

ftxui::Element PartyBattleScreen::render_party_log_panel(int width,
                                                         int height) {
  using namespace ftxui;

  auto &parties = ctx_.account_data().parties();
  if (parties.empty()) {
    return window(text("Party log") | bold, text("No selected party.")) |
           size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height);
  }

  const auto selected = std::min(party_index_, parties.size() - 1);
  for (std::size_t i = 0; i < parties.size(); ++i) {
    parties[i].log().set_focused(i == selected && focused_log_ == 1);
  }

  auto &party = parties[selected];
  return window(text(std::string(party.name()) + " log") | bold,
                party.log().Render() | yframe | vscroll_indicator) |
         size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height);
}

ftxui::Element PartyBattleScreen::render_question_panel(int width,
                                                        int height) const {
  using namespace ftxui;

  std::vector<Element> lines;
  lines.reserve(static_cast<std::size_t>(std::max(0, height)));
  for (int i = 0; i < height; ++i) {
    lines.push_back(text("????????????????????????????????????????????????"));
  }

  return vbox(std::move(lines)) | size(WIDTH, EQUAL, width) |
         size(HEIGHT, EQUAL, height);
}

} // namespace fl::widgets
