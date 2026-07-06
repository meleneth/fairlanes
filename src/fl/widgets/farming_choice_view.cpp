#include "fl/widgets/farming_choice_view.hpp"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "fl/lospec500.hpp"
#include "fl/primitives/farming_plan.hpp"
#include "fl/primitives/party_data.hpp"

namespace fl::widgets {
namespace {
constexpr std::size_t kChoiceCount = 5;

std::size_t index_for_focus(fl::primitives::FarmFocus focus) noexcept {
  const auto definitions = fl::primitives::all_farm_focus_definitions();
  const auto it = std::find_if(
      definitions.begin(), definitions.end(),
      [focus](const auto &definition) { return definition.focus == focus; });
  if (it == definitions.end()) {
    return 0;
  }
  return static_cast<std::size_t>(std::distance(definitions.begin(), it));
}

std::string plan_summary_text(const fl::primitives::FarmingPlan &plan) {
  return std::string{"Farm: "} +
         std::string{fl::primitives::display_name(plan.focus)} + " / " +
         std::string{fl::primitives::display_name(plan.reward_class)};
}

} // namespace

fl::primitives::FarmFocus FarmingChoiceState::selected_focus() const noexcept {
  const auto definitions = fl::primitives::all_farm_focus_definitions();
  return definitions[std::min(selected_index_, definitions.size() - 1)].focus;
}

void FarmingChoiceState::next() noexcept {
  selected_index_ = (selected_index_ + 1) % kChoiceCount;
}

void FarmingChoiceState::previous() noexcept {
  selected_index_ = (selected_index_ + kChoiceCount - 1) % kChoiceCount;
}

bool FarmingChoiceState::set_from_digit(char digit) noexcept {
  if (digit < '1' || digit > '5') {
    return false;
  }
  selected_index_ = static_cast<std::size_t>(digit - '1');
  return true;
}

void FarmingChoiceState::set_focus(fl::primitives::FarmFocus focus) noexcept {
  selected_index_ = index_for_focus(focus);
}

FarmingChoiceView::FarmingChoiceView(
    fl::primitives::PartyData &party,
    fl::primitives::GrimoireDiscipline discipline)
    : party_{&party}, discipline_{discipline},
      advice_{party.farming_choice_advice()} {
  initialize_selection_from_mode();
}

bool FarmingChoiceView::OnEvent(ftxui::Event event) {
  if (party_ == nullptr || !party_->needs_farm_focus_choice()) {
    return false;
  }

  if (event == ftxui::Event::ArrowDown ||
      event == ftxui::Event::Character("j")) {
    state_.next();
    return true;
  }

  if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character("k")) {
    state_.previous();
    return true;
  }

  if (event == ftxui::Event::Return) {
    apply_selected_focus();
    return true;
  }

  for (char digit = '1'; digit <= '5'; ++digit) {
    if (event == ftxui::Event::Character(std::string(1, digit))) {
      state_.set_from_digit(digit);
      apply_selected_focus();
      return true;
    }
  }

  return false;
}

ftxui::Element FarmingChoiceView::Render() {
  using namespace ftxui;

  if (party_ == nullptr) {
    return text("No party selected.");
  }

  if (!party_->needs_farm_focus_choice()) {
    return render_farming_plan_summary(party_->farming_plan(), 48);
  }

  const auto definitions = fl::primitives::all_farm_focus_definitions();
  std::vector<Element> rows;
  rows.reserve(definitions.size() + 3);
  rows.push_back(text("Choose where this party is farming") | bold |
                 fl::lospec500::on_not_black(fl::lospec500::color_at(32)));
  rows.push_back(text("Mode: " +
                      std::string{fl::primitives::display_name(
                          party_->progression_control_mode())} +
                      "  Discipline: " +
                      std::string{fl::primitives::display_name(discipline_)}) |
                 fl::lospec500::on_not_black(fl::lospec500::color_at(30)));
  rows.push_back(text("Autotree: " +
                      std::string{fl::primitives::display_name(
                          advice_.recommended_focus)} +
                      " - " + std::string{advice_.recommendation_reason}) |
                 fl::lospec500::on_not_black(fl::lospec500::color_at(19)));

  for (std::size_t i = 0; i < definitions.size(); ++i) {
    const auto &definition = definitions[i];
    const bool selected = i == state_.selected_index();
    const bool recommended = definition.focus == advice_.recommended_focus;
    const bool previous = advice_.previous_focus.has_value() &&
                          definition.focus == *advice_.previous_focus;
    std::string markers;
    if (recommended) {
      markers += " [auto]";
    }
    if (previous) {
      markers += " [last]";
    }
    auto line = text((selected ? "> " : "  ") + std::to_string(i + 1) + ". " +
                     std::string{definition.display_name} + markers + " - " +
                     std::string{definition.description});
    if (selected) {
      line = line | bold | bgcolor(fl::lospec500::color_at(4)) |
             fl::lospec500::on_not_black(fl::lospec500::color_at(32));
    } else if (recommended) {
      line = line | bold |
             fl::lospec500::on_not_black(fl::lospec500::color_at(19));
    } else if (previous) {
      line = line | fl::lospec500::on_not_black(fl::lospec500::color_at(30));
    } else {
      line = line | fl::lospec500::on_not_black(fl::lospec500::color_at(29));
    }
    rows.push_back(line);
  }

  const auto preview =
      fl::primitives::make_farming_plan(discipline_, state_.selected_focus());
  rows.push_back(
      text("Reward path: " +
           std::string{fl::primitives::display_name(preview.reward_class)}) |
      fl::lospec500::on_not_black(fl::lospec500::color_at(19)));

  return window(text("Farming Focus") | bold, vbox(std::move(rows))) |
         bgcolor(fl::lospec500::color_at(0)) |
         fl::lospec500::on_not_black(fl::lospec500::color_at(32));
}

void FarmingChoiceView::initialize_selection_from_mode() {
  if (party_ == nullptr) {
    return;
  }

  switch (party_->progression_control_mode()) {
  case fl::primitives::ProgressionControlMode::Auto:
  case fl::primitives::ProgressionControlMode::Guided:
    state_.set_focus(advice_.recommended_focus);
    break;
  case fl::primitives::ProgressionControlMode::Manual:
    if (advice_.previous_focus.has_value()) {
      state_.set_focus(*advice_.previous_focus);
    }
    break;
  }
}

void FarmingChoiceView::apply_selected_focus() {
  if (party_ == nullptr) {
    return;
  }
  party_->select_farming_plan(discipline_, state_.selected_focus());
}

ftxui::Element
render_farming_plan_summary(const fl::primitives::FarmingPlan &plan,
                            int width) {
  using namespace ftxui;
  return text(plan_summary_text(plan)) | size(WIDTH, EQUAL, width) |
         fl::lospec500::on_not_black(fl::lospec500::color_at(19));
}

} // namespace fl::widgets
