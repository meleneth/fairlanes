#include "party_view.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/ecs/components/is_account.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/battle_render_budget.hpp"
#include "fl/widgets/combatant.hpp"
#include "fl/widgets/farming_choice_view.hpp"
#include "fl/widgets/inventory_list.hpp"
#include "fl/widgets/party_status.hpp"
#include "fl/widgets/player_details_pane.hpp"
#include "fl/widgets/textures/bog_background.hpp"
#include "fl/widgets/textures/forest_background.hpp"
#include "fl/widgets/textures/savannah_background.hpp"

namespace fl::widgets {

struct PartyViewLayout {
  int screen_width{1};
  int active_height{1};
  int battle_width{1};
  int inventory_width{24};
  int title_height{1};
  int stage_height{10};
  int combatant_row_height{5};
  int bottom_height{8};
  int log_width{1};
  int details_width{28};
};

namespace {

constexpr int kRootChromeHeight = 2;
constexpr int kCombatantColumns = 5;
constexpr int kSeparatorWidth = 1;
constexpr int kSeparatorHeight = 1;
constexpr int kMinInventoryWidth = 24;
constexpr int kMaxInventoryWidth = 44;
constexpr int kMinCombatantRowHeight = 5;
constexpr int kMinBottomHeight = 8;
constexpr int kMinDetailsWidth = 28;
constexpr int kMaxDetailsWidth = 44;

PartyViewLayout current_layout(const BattleRenderBudget &budget,
                               int stage_row_count) {
  PartyViewLayout layout;
  layout.screen_width = budget.requested_width;
  layout.active_height = std::max(1, budget.requested_height -
                                         kRootChromeHeight);

  const int preferred_inventory = std::clamp(
      layout.screen_width / 3, kMinInventoryWidth, kMaxInventoryWidth);
  layout.inventory_width =
      std::min(preferred_inventory, std::max(1, layout.screen_width - 1));
  layout.battle_width = std::max(
      1, layout.screen_width - layout.inventory_width - kSeparatorWidth);

  const int fixed_height = layout.title_height + kSeparatorHeight;
  const int variable_height = std::max(1, layout.active_height - fixed_height);
  const int preferred_bottom = std::max(kMinBottomHeight, variable_height / 3);
  layout.bottom_height = std::min(variable_height, preferred_bottom);

  const int rows = std::max(1, stage_row_count);
  layout.stage_height = std::max(1, variable_height - layout.bottom_height);
  layout.combatant_row_height = std::max(1, layout.stage_height / rows);
  if (layout.stage_height >= rows * kMinCombatantRowHeight) {
    layout.combatant_row_height =
        std::max(kMinCombatantRowHeight, layout.combatant_row_height);
  }
  layout.stage_height = layout.combatant_row_height * rows;
  layout.bottom_height = std::max(
      1, layout.active_height - fixed_height - layout.stage_height);

  layout.details_width = std::clamp(layout.battle_width / 3, kMinDetailsWidth,
                                    kMaxDetailsWidth);
  layout.details_width =
      std::min(layout.details_width, std::max(1, layout.battle_width - 1));
  layout.log_width =
      std::max(1, layout.battle_width - layout.details_width - kSeparatorWidth);
  return layout;
}

std::uint32_t texture_seed_for_party(std::string_view party_name,
                                     std::size_t party_index,
                                     std::uint32_t salt) {
  std::uint32_t seed = 0x705A9E31u ^ salt ^
                       static_cast<std::uint32_t>((party_index + 1U) *
                                                  0x9E3779B9u);
  for (unsigned char ch : party_name) {
    seed ^= static_cast<std::uint32_t>(ch);
    seed *= 0x85EBCA6Bu;
    seed ^= seed >> 13;
  }
  return seed;
}

ftxui::Element textured_stage_panel(ftxui::Element foreground, int width,
                                    int height, std::uint32_t seed,
                                    std::size_t party_index,
                                    bool show_field_ambience) {
  using namespace ftxui;

  foreground = std::move(foreground) | size(WIDTH, EQUAL, width) |
               size(HEIGHT, EQUAL, height);
  if (!show_field_ambience) {
    return foreground;
  }

  switch (party_index % 3) {
  case 1:
    return textures::SavannahPanel(std::move(foreground), width, height, seed);
  case 2:
    return textures::BogPanel(std::move(foreground), width, height, seed);
  default:
    return textures::ForestPanel(std::move(foreground), width, height, seed);
  }
}

std::string entity_label(entt::entity entity) {
  return "#" + std::to_string(
                   static_cast<std::underlying_type_t<entt::entity>>(entity));
}

} // namespace

PartyView::PartyView(fl::context::AccountCtx context, std::size_t party_index)
    : ctx_(std::move(context)), party_index_(party_index) {
  if (party_index_ >= ctx_.account_data().parties().size()) {
    return;
  }

  auto &party = ctx_.account_data().party(party_index_);
  farming_choice_ =
      ftxui::Make<FarmingChoiceView>(party, party.grimoire_discipline());
  inventory_list_ = ftxui::Make<InventoryList>(ctx_.reg(), party);
  player_details_ = ftxui::Make<PlayerDetailsPane>(ctx_.reg(), party);
  Add(farming_choice_);
  Add(inventory_list_);
  Add(player_details_);
  set_focus(FocusPane::inventory);
}

bool PartyView::OnEvent(ftxui::Event event) {
  auto &parties = ctx_.account_data().parties();
  if (party_index_ >= parties.size()) {
    return false;
  }

  if (parties[party_index_].needs_farm_focus_choice() && farming_choice_ &&
      farming_choice_->OnEvent(event)) {
    return true;
  }

  if (event == ftxui::Event::Tab) {
    switch (focus_) {
    case FocusPane::inventory:
      set_focus(FocusPane::player_details);
      break;
    case FocusPane::player_details:
      set_focus(FocusPane::party_log);
      break;
    case FocusPane::party_log:
      set_focus(FocusPane::inventory);
      break;
    }
    return true;
  }

  if (event == ftxui::Event::TabReverse) {
    switch (focus_) {
    case FocusPane::inventory:
      set_focus(FocusPane::party_log);
      break;
    case FocusPane::player_details:
      set_focus(FocusPane::inventory);
      break;
    case FocusPane::party_log:
      set_focus(FocusPane::player_details);
      break;
    }
    return true;
  }

  if (focus_ == FocusPane::inventory && inventory_list_) {
    return inventory_list_->OnEvent(event);
  }

  if (focus_ == FocusPane::player_details && player_details_) {
    return player_details_->OnEvent(event);
  }

  if (focus_ == FocusPane::party_log) {
    return parties[party_index_].log().OnEvent(event);
  }

  return false;
}

ftxui::Element PartyView::Render() {
  using namespace ftxui;

  auto *is_account =
      ctx_.reg().try_get<fl::ecs::components::IsAccount>(ctx_.self());
  if (!is_account) {
    return window(text("PartyView"), text("ctx_.self() is not an account")) |
           bgcolor(fl::lospec500::color_at(0)) |
           color(fl::lospec500::color_at(32));
  }

  auto &parties = is_account->account_data().parties();
  if (party_index_ >= parties.size()) {
    return window(text("PartyView"),
                  text("No party at index " + std::to_string(party_index_))) |
           bgcolor(fl::lospec500::color_at(0)) |
           color(fl::lospec500::color_at(32));
  }

  const auto budget = current_battle_render_budget();
  auto &party = parties[party_index_];
  const int stage_rows = party.has_encounter() ? 2 : 2;
  const auto layout = current_layout(budget, stage_rows);

  auto inventory = inventory_list_ ? inventory_list_->Render()
                                   : text("No inventory.");
  inventory = std::move(inventory) |
              size(WIDTH, EQUAL, layout.inventory_width) |
              size(HEIGHT, EQUAL, layout.active_height);

  return hbox({
             render_party(layout, budget) |
                 size(WIDTH, EQUAL, layout.battle_width) |
                 size(HEIGHT, EQUAL, layout.active_height),
             separator() | size(WIDTH, EQUAL, kSeparatorWidth) |
                 size(HEIGHT, EQUAL, layout.active_height),
             std::move(inventory),
         }) |
         size(WIDTH, EQUAL, layout.screen_width) |
         size(HEIGHT, EQUAL, layout.active_height) |
         bgcolor(fl::lospec500::color_at(0)) |
         color(fl::lospec500::color_at(32));
}

ftxui::Element PartyView::render_party(const PartyViewLayout &layout,
                                       const BattleRenderBudget &budget) {
  using namespace ftxui;

  auto &party = ctx_.account_data().party(party_index_);
  std::vector<Element> rows;
  rows.reserve(5);

  const int cell_width = std::max(1, layout.battle_width / kCombatantColumns);
  const int row_remainder =
      std::max(0, layout.battle_width - (cell_width * kCombatantColumns));

  auto finish_row = [&](std::vector<Element> cells) {
    while (cells.size() < static_cast<std::size_t>(kCombatantColumns)) {
      cells.push_back(filler() | size(WIDTH, EQUAL, cell_width) |
                      size(HEIGHT, EQUAL, layout.combatant_row_height));
    }
    if (row_remainder > 0) {
      cells.push_back(filler() | size(WIDTH, EQUAL, row_remainder) |
                      size(HEIGHT, EQUAL, layout.combatant_row_height));
    }
    return hbox(std::move(cells)) | size(WIDTH, EQUAL, layout.battle_width) |
           size(HEIGHT, EQUAL, layout.combatant_row_height);
  };

  auto render_entity_row = [&](const std::vector<entt::entity> &entities,
                               std::uint32_t salt) {
    std::vector<Element> cells;
    cells.reserve(kCombatantColumns + 1);

    for (auto entity : entities) {
      if (cells.size() >= static_cast<std::size_t>(kCombatantColumns)) {
        break;
      }
      Element cell = filler();
      if (ctx_.reg().valid(entity)) {
        cell = Combatant{ctx_.reg(), entity, true}.Render() |
               bgcolor(fl::lospec500::color_at(0)) | clear_under;
      }
      cells.push_back(std::move(cell) | size(WIDTH, EQUAL, cell_width) |
                      size(HEIGHT, EQUAL, layout.combatant_row_height));
    }

    return textured_stage_panel(
        finish_row(std::move(cells)), layout.battle_width,
        layout.combatant_row_height,
        texture_seed_for_party(party.name(), party_index_, salt), party_index_,
        budget.show_field_ambience);
  };

  auto render_members = [&] {
    std::vector<Element> cells;
    cells.reserve(kCombatantColumns + 1);

    const bool in_combat = party.has_encounter();
    for (const auto &member : party.members()) {
      if (cells.size() >= static_cast<std::size_t>(kCombatantColumns)) {
        break;
      }
      auto cell = Combatant{ctx_.reg(), member.member_id(), in_combat}.Render() |
                  bgcolor(fl::lospec500::color_at(0)) | clear_under;
      cells.push_back(std::move(cell) | size(WIDTH, EQUAL, cell_width) |
                      size(HEIGHT, EQUAL, layout.combatant_row_height));
    }

    return textured_stage_panel(
        finish_row(std::move(cells)), layout.battle_width,
        layout.combatant_row_height,
        texture_seed_for_party(party.name(), party_index_, 0x5F3759DFu),
        party_index_, budget.show_field_ambience);
  };

  rows.push_back(
      text(std::string(party.name()) + " " + entity_label(party.party_id())) |
      bold | size(WIDTH, EQUAL, layout.battle_width) |
      size(HEIGHT, EQUAL, layout.title_height));

  if (party.needs_farm_focus_choice() && farming_choice_) {
    rows.push_back(farming_choice_->Render() |
                   size(WIDTH, EQUAL, layout.battle_width) |
                   size(HEIGHT, EQUAL, layout.combatant_row_height * 2));
  } else if (party.has_encounter()) {
    auto &encounter = party.encounter_data();
    rows.push_back(
        render_entity_row(encounter.attackers().members(), 0xA77A0011u));
    rows.push_back(
        render_entity_row(encounter.defenders().members(), 0xDEFED123u));
  } else {
    rows.push_back(textured_stage_panel(
        vbox({PartyStatus{party}.Render(),
              render_farming_plan_summary(party.farming_plan(),
                                          layout.battle_width)}) |
            size(WIDTH, EQUAL, layout.battle_width) |
            size(HEIGHT, EQUAL, layout.combatant_row_height),
        layout.battle_width, layout.combatant_row_height,
        texture_seed_for_party(party.name(), party_index_, 0x57A70500u),
        party_index_, budget.show_field_ambience));
    rows.push_back(render_members());
  }

  rows.push_back(separator() | size(WIDTH, EQUAL, layout.battle_width) |
                 size(HEIGHT, EQUAL, kSeparatorHeight));

  auto party_log = party.log().Render() | yframe | vscroll_indicator |
                   size(WIDTH, EQUAL, layout.log_width) |
                   size(HEIGHT, EQUAL, layout.bottom_height);
  auto details = player_details_ ? player_details_->Render()
                                 : text("No player details.");
  details = std::move(details) | size(WIDTH, EQUAL, layout.details_width) |
            size(HEIGHT, EQUAL, layout.bottom_height);
  rows.push_back(hbox({
                     std::move(party_log),
                     separator() | size(WIDTH, EQUAL, kSeparatorWidth) |
                         size(HEIGHT, EQUAL, layout.bottom_height),
                     std::move(details),
                 }) |
                 size(WIDTH, EQUAL, layout.battle_width) |
                 size(HEIGHT, EQUAL, layout.bottom_height));

  return vbox(std::move(rows)) | size(WIDTH, EQUAL, layout.battle_width) |
         size(HEIGHT, EQUAL, layout.active_height);
}

void PartyView::set_focus(FocusPane focus) {
  focus_ = focus;
  if (party_index_ < ctx_.account_data().parties().size()) {
    ctx_.account_data()
        .party(party_index_)
        .log()
        .set_focused(focus_ == FocusPane::party_log);
  }
  if (inventory_list_) {
    inventory_list_->set_focused(focus_ == FocusPane::inventory);
  }
  if (player_details_) {
    player_details_->set_focused(focus_ == FocusPane::player_details);
  }
}

} // namespace fl::widgets
