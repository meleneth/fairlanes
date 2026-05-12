#include "party_view.hpp"

#include <algorithm>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <entt/entt.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/is_account.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/combatant.hpp"
#include "fl/widgets/party_status.hpp"

namespace fl::widgets {

namespace {

std::string entity_label(entt::entity entity) {
  return "#" + std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
                   entity));
}

} // namespace

PartyView::PartyView(fl::context::AccountCtx context, std::size_t party_index)
    : ctx_(std::move(context)), party_index_(party_index) {}

bool PartyView::OnEvent(ftxui::Event event) {
  auto &parties = ctx_.account_data().parties();
  if (party_index_ >= parties.size()) {
    return false;
  }

  const int item_count = static_cast<int>(parties[party_index_].items().size());
  if (item_count <= 0) {
    inventory_cursor_ = 0;
    return false;
  }

  if (event == ftxui::Event::ArrowDown ||
      event == ftxui::Event::Character("j")) {
    inventory_cursor_ = std::min(inventory_cursor_ + 1, item_count - 1);
    return true;
  }

  if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character("k")) {
    inventory_cursor_ = std::max(inventory_cursor_ - 1, 0);
    return true;
  }

  if (event == ftxui::Event::PageDown) {
    inventory_cursor_ = std::min(inventory_cursor_ + 8, item_count - 1);
    return true;
  }

  if (event == ftxui::Event::PageUp) {
    inventory_cursor_ = std::max(inventory_cursor_ - 8, 0);
    return true;
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
             render_inventory() | size(WIDTH, GREATER_THAN, 30) | flex,
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
      cells.push_back(Combatant{ctx_.reg(), entity}.Render() | xflex);
    }

    while (cells.size() < 5) {
      cells.push_back(filler() | xflex);
    }

    return hbox(std::move(cells));
  };

  auto render_members = [&] {
    std::vector<Element> cells;
    cells.reserve(5);

    for (const auto &member : party.members()) {
      if (cells.size() >= 5) {
        break;
      }
      cells.push_back(Combatant{ctx_.reg(), member.member_id()}.Render() |
                      xflex);
    }

    while (cells.size() < 5) {
      cells.push_back(filler() | xflex);
    }

    return hbox(std::move(cells));
  };

  rows.push_back(text(std::string(party.name()) + " " +
                      entity_label(party.party_id())) |
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
  rows.push_back(party.log().Render() | yframe | vscroll_indicator | flex);

  return vbox(std::move(rows)) | flex;
}

ftxui::Element PartyView::render_inventory() {
  using namespace ftxui;

  auto &party = ctx_.account_data().party(party_index_);
  auto items = party.items();
  const int item_count = static_cast<int>(items.size());
  if (item_count <= 0) {
    inventory_cursor_ = 0;
    return window(text("Inventory"), text("No items."));
  }

  inventory_cursor_ = std::clamp(inventory_cursor_, 0, item_count - 1);

  Elements lines;
  lines.reserve(items.size());

  for (int i = 0; i < item_count; ++i) {
    const auto item = items[static_cast<std::size_t>(i)];
    std::string label = entity_label(item);

    if (auto *equipment =
            ctx_.reg().try_get<fl::ecs::components::Equipment>(item)) {
      label = std::string(equipment->name()) + " [" +
              std::string(fl::ecs::components::to_string(equipment->slot())) +
              "]";
    } else if (auto *stats =
                   ctx_.reg().try_get<fl::ecs::components::Stats>(item)) {
      label = stats->name_;
    }

    auto line = text(label);
    if (i == inventory_cursor_) {
      line = line | inverted | focusPosition(0, i);
    }
    lines.push_back(line);
  }

  return window(text("Inventory"), vbox(std::move(lines)) | yframe |
                                       vscroll_indicator | flex);
}

} // namespace fl::widgets
