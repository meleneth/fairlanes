#include "all_account_battle_screen.hpp"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "fl/ecs/components/stats.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/primitives/member_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/battle_render_budget.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::widgets {
namespace {

std::string shorten(std::string value, std::size_t max_width) {
  if (value.size() <= max_width) {
    return value;
  }

  if (max_width <= 1) {
    return value.substr(0, max_width);
  }

  value.resize(max_width - 1);
  value += "~";
  return value;
}

ftxui::Decorator state_color(fl::primitives::PartyData &party) {
  if (party.has_encounter()) {
    return ftxui::color(fl::lospec500::color_at(20)) | ftxui::bold;
  }
  if (party.all_members_dead()) {
    return ftxui::color(fl::lospec500::color_at(6)) | ftxui::bold;
  }
  if (party.town_penalty_active()) {
    return ftxui::color(fl::lospec500::color_at(27)) | ftxui::bold;
  }
  return ftxui::color(fl::lospec500::color_at(14));
}

std::string state_label(fl::primitives::PartyData &party) {
  if (party.has_encounter()) {
    return "FIGHT";
  }
  if (party.all_members_dead()) {
    return "DEAD ";
  }
  if (party.town_penalty_active()) {
    return "TOWN ";
  }
  return "ROAM ";
}

} // namespace

AllAccountBattleScreen::AllAccountBattleScreen(
    entt::registry &registry,
    std::deque<fl::primitives::AccountData> &accounts)
    : registry_(&registry), accounts_(&accounts) {}

bool AllAccountBattleScreen::OnEvent(ftxui::Event event) {
  if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character("k")) {
    select_relative(-1);
    return true;
  }

  if (event == ftxui::Event::ArrowDown ||
      event == ftxui::Event::Character("j")) {
    select_relative(1);
    return true;
  }

  return false;
}

ftxui::Element AllAccountBattleScreen::Render() {
  using namespace ftxui;

  const auto budget = current_battle_render_budget();
  const int width = std::max(40, budget.requested_width);
  const int height = std::max(12, budget.requested_height - 3);
  const int detail_height = std::clamp(height / 3, 6, 14);
  const int overview_height = std::max(3, height - detail_height);

  return vbox({
             render_party_overview(width, overview_height),
             render_selected_party_detail(width, detail_height),
         }) |
         size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height) |
         bgcolor(fl::lospec500::color_at(0)) |
         color(fl::lospec500::color_at(32));
}

void AllAccountBattleScreen::select_relative(int delta) {
  const auto total = total_party_count();
  if (total == 0) {
    selected_account_index_ = 0;
    selected_party_index_ = 0;
    return;
  }

  const auto current = selected_flat_index();
  const auto next =
      (current + total + static_cast<std::size_t>(delta < 0 ? total - 1 : 1)) %
      total;
  select_flat_index(next);
}

std::size_t AllAccountBattleScreen::total_party_count() const noexcept {
  if (accounts_ == nullptr) {
    return 0;
  }

  std::size_t total = 0;
  for (const auto &account : *accounts_) {
    total += account.parties().size();
  }
  return total;
}

std::size_t AllAccountBattleScreen::selected_flat_index() const noexcept {
  if (accounts_ == nullptr) {
    return 0;
  }

  std::size_t flat = 0;
  for (std::size_t account_index = 0; account_index < accounts_->size();
       ++account_index) {
    const auto &parties = accounts_->at(account_index).parties();
    if (account_index == selected_account_index_) {
      return flat + std::min(selected_party_index_,
                             parties.empty() ? 0 : parties.size() - 1);
    }
    flat += parties.size();
  }

  return 0;
}

void AllAccountBattleScreen::select_flat_index(std::size_t flat_index) noexcept {
  if (accounts_ == nullptr) {
    selected_account_index_ = 0;
    selected_party_index_ = 0;
    return;
  }

  for (std::size_t account_index = 0; account_index < accounts_->size();
       ++account_index) {
    const auto party_count = accounts_->at(account_index).parties().size();
    if (flat_index < party_count) {
      selected_account_index_ = account_index;
      selected_party_index_ = flat_index;
      return;
    }
    flat_index -= party_count;
  }

  selected_account_index_ = 0;
  selected_party_index_ = 0;
}

ftxui::Element AllAccountBattleScreen::render_party_overview(int width,
                                                             int height) {
  using namespace ftxui;

  std::vector<Element> rows;
  rows.reserve(accounts_ ? accounts_->size() * 5 : 1);

  if (accounts_ == nullptr || registry_ == nullptr || accounts_->empty()) {
    rows.push_back(text("Chaos Attract: no simulation."));
  } else {
    for (std::size_t account_index = 0; account_index < accounts_->size();
         ++account_index) {
      auto &account = accounts_->at(account_index);
      auto &parties = account.parties();
      for (std::size_t party_index = 0; party_index < parties.size();
           ++party_index) {
        const bool selected = account_index == selected_account_index_ &&
                              party_index == selected_party_index_;
        rows.push_back(render_party_row(account_index, party_index,
                                        parties[party_index], width, selected));
      }
    }
  }

  if (rows.empty()) {
    rows.push_back(text("No parties."));
  }

  const int visible_rows = std::max(1, height - 2);
  const auto selected = selected_flat_index();
  const auto total = rows.size();
  const auto half_window = static_cast<std::size_t>(visible_rows / 2);
  std::size_t start = selected > half_window ? selected - half_window : 0;
  if (start + static_cast<std::size_t>(visible_rows) > total) {
    start = total > static_cast<std::size_t>(visible_rows)
                ? total - static_cast<std::size_t>(visible_rows)
                : 0;
  }

  std::vector<Element> visible;
  visible.reserve(static_cast<std::size_t>(visible_rows));
  for (std::size_t i = 0; i < static_cast<std::size_t>(visible_rows); ++i) {
    const auto row_index = start + i;
    visible.push_back(row_index < rows.size() ? rows[row_index] : text(""));
  }

  return window(text("Chaos Attract: all accounts") | bold,
                vbox(std::move(visible))) |
         size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height);
}

ftxui::Element AllAccountBattleScreen::render_party_row(
    std::size_t account_index, std::size_t party_index,
    fl::primitives::PartyData &party, int width, bool selected) const {
  using namespace ftxui;

  const int marker_width = 2;
  const int left_width = std::clamp(width / 5, 18, 30);
  const int state_width = 7;
  const int roster_width =
      std::max(8, (width - marker_width - left_width - state_width) / 2);

  auto label = "A" + std::to_string(account_index + 1) + " P" +
               std::to_string(party_index + 1) + " " +
               std::string{party.name()};

  Element enemies = text("");
  Element heroes = text("");
  if (party.has_encounter()) {
    auto &encounter = party.encounter_data();
    enemies = render_roster(encounter.attackers().members(), roster_width);
    heroes = render_roster(encounter.defenders().members(), roster_width);
  } else {
    enemies = text(shorten("field / town", static_cast<std::size_t>(roster_width))) |
              color(fl::lospec500::color_at(24));
    heroes = render_member_roster(party.members(), roster_width);
  }

  auto row = hbox({
      text(selected ? "* " : "  ") | color(fl::lospec500::color_at(8)) |
          bold | size(WIDTH, EQUAL, marker_width),
      text(shorten(label, static_cast<std::size_t>(left_width))) |
          color(fl::lospec500::color_at(15)) | size(WIDTH, EQUAL, left_width),
      text(state_label(party)) | state_color(party) |
          size(WIDTH, EQUAL, state_width),
      enemies | size(WIDTH, EQUAL, roster_width),
      text(" -> ") | color(fl::lospec500::color_at(24)),
      heroes | flex,
  });

  if (selected) {
    row = row | bgcolor(fl::lospec500::color_at(1));
  }

  return row | size(HEIGHT, EQUAL, 1) | clear_under;
}

ftxui::Element AllAccountBattleScreen::render_selected_party_detail(int width,
                                                                    int height) {
  using namespace ftxui;

  if (accounts_ == nullptr || accounts_->empty() ||
      selected_account_index_ >= accounts_->size()) {
    return window(text("Selected party") | bold, text("No selected party.")) |
           size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height);
  }

  auto &account = accounts_->at(selected_account_index_);
  auto &parties = account.parties();
  if (parties.empty()) {
    return window(text("Selected party") | bold, text("No parties.")) |
           size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height);
  }

  selected_party_index_ = std::min(selected_party_index_, parties.size() - 1);
  auto &party = parties[selected_party_index_];
  const int summary_width = std::max(24, width / 2);
  const int log_width = std::max(1, width - summary_width);

  std::vector<Element> summary_lines;
  summary_lines.reserve(4);
  summary_lines.push_back(
      text("A" + std::to_string(selected_account_index_ + 1) + " P" +
           std::to_string(selected_party_index_ + 1) + " " +
           std::string{party.name()}) |
      bold | color(fl::lospec500::color_at(15)));
  summary_lines.push_back(text(state_label(party)) | state_color(party));
  if (party.has_encounter()) {
    auto &encounter = party.encounter_data();
    summary_lines.push_back(hbox({
        text("Enemy ") | color(fl::lospec500::color_at(24)),
        render_roster(encounter.attackers().members(), summary_width - 8),
    }));
    summary_lines.push_back(hbox({
        text("Party ") | color(fl::lospec500::color_at(24)),
        render_roster(encounter.defenders().members(), summary_width - 8),
    }));
  } else {
    summary_lines.push_back(hbox({
        text("Party ") | color(fl::lospec500::color_at(24)),
        render_member_roster(party.members(), summary_width - 8),
    }));
    summary_lines.push_back(text("Waiting for the next fight") |
                            color(fl::lospec500::color_at(24)));
  }

  party.log().set_focused(false);
  return hbox({
             window(text("Selected battle") | bold,
                    vbox(std::move(summary_lines))),
             window(text(std::string{party.name()} + " log") | bold,
                    party.log().Render() | yframe | vscroll_indicator) |
                 size(WIDTH, EQUAL, log_width),
         }) |
         size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height);
}

ftxui::Element AllAccountBattleScreen::render_roster(
    std::span<const entt::entity> entities, int width) const {
  using namespace ftxui;

  std::string out;
  for (auto entity : entities) {
    if (!out.empty()) {
      out += " ";
    }
    out += entity_chip(entity);
  }

  if (out.empty()) {
    out = "-";
  }

  return text(shorten(out, static_cast<std::size_t>(width))) |
         color(fl::lospec500::color_at(32));
}

ftxui::Element AllAccountBattleScreen::render_member_roster(
    const std::deque<fl::primitives::MemberData> &members, int width) const {
  std::vector<entt::entity> entities;
  entities.reserve(members.size());
  for (const auto &member : members) {
    entities.push_back(member.member_id());
  }
  return render_roster(entities, width);
}

std::string AllAccountBattleScreen::entity_chip(entt::entity entity) const {
  namespace c = fl::ecs::components;

  if (entity == entt::null || registry_ == nullptr) {
    return "-";
  }

  const auto *stats = registry_->try_get<c::Stats>(entity);
  if (stats == nullptr) {
    return "?";
  }

  std::string name = stats->name_;
  name = shorten(name, 6);
  return name + "(" + std::to_string(std::max(0, stats->hp_)) + ")";
}

} // namespace fl::widgets
