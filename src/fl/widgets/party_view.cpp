#include "party_view.hpp"

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
#include "fl/widgets/combatant.hpp"
#include "fl/widgets/inventory_list.hpp"
#include "fl/widgets/party_status.hpp"
#include "fl/widgets/player_details_pane.hpp"

namespace fl::widgets {

namespace {

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
  inventory_list_ = ftxui::Make<InventoryList>(ctx_.reg(), party);
  player_details_ = ftxui::Make<PlayerDetailsPane>(ctx_.reg(), party);
  Add(inventory_list_);
  Add(player_details_);
  set_focus(FocusPane::inventory);
}

bool PartyView::OnEvent(ftxui::Event event) {
  auto &parties = ctx_.account_data().parties();
  if (party_index_ >= parties.size()) {
    return false;
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

  return hbox({
             render_party() | flex,
             separator(),
             inventory_list_ ? inventory_list_->Render() |
                                   size(WIDTH, GREATER_THAN, 30) | flex
                             : text("No inventory.") | flex,
         }) |
         bgcolor(fl::lospec500::color_at(0)) |
         color(fl::lospec500::color_at(32));
}

ftxui::Element PartyView::render_party() {
  using namespace ftxui;

  auto &party = ctx_.account_data().party(party_index_);
  std::vector<Element> rows;

  auto render_entity_row = [&](const std::vector<entt::entity> &entities) {
    std::vector<Element> cells;
    cells.reserve(5);

    for (auto entity : entities) {
      if (cells.size() >= 5) {
        break;
      }
      cells.push_back(
          Combatant{ctx_.reg(), entity, true}.Render() | xflex);
    }

    while (cells.size() < 5) {
      cells.push_back(filler() | xflex);
    }

    return hbox(std::move(cells));
  };

  auto render_members = [&] {
    std::vector<Element> cells;
    cells.reserve(5);

    const bool in_combat = party.has_encounter();
    for (const auto &member : party.members()) {
      if (cells.size() >= 5) {
        break;
      }
      cells.push_back(
          Combatant{ctx_.reg(), member.member_id(), in_combat}.Render() |
          xflex);
    }

    while (cells.size() < 5) {
      cells.push_back(filler() | xflex);
    }

    return hbox(std::move(cells));
  };

  rows.push_back(
      text(std::string(party.name()) + " " + entity_label(party.party_id())) |
      bold);

  if (party.has_encounter()) {
    auto &encounter = party.encounter_data();
    rows.push_back(render_entity_row(encounter.attackers().members()));
    rows.push_back(render_entity_row(encounter.defenders().members()));
  } else {
    rows.push_back(PartyStatus{party}.Render());
    rows.push_back(render_members());
  }

  rows.push_back(separator());
  rows.push_back(hbox({
                     party.log().Render() | yframe | vscroll_indicator | flex,
                     separator(),
                     player_details_ ? player_details_->Render() |
                                           size(WIDTH, GREATER_THAN, 28) |
                                           size(WIDTH, LESS_THAN, 44)
                                     : text("No player details."),
                 }) |
                 flex);

  return vbox(std::move(rows)) | flex;
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
