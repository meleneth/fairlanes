#include "fl/widgets/bestiary_view.hpp"

#include "fl/generated/monster_content.hpp"
#include <algorithm>
#include <utility>

namespace fl::widgets {

BestiaryView::BestiaryView(const fl::primitives::DiscoveryJournal &journal,
                           OpenSkill open,
                           std::optional<fl::monster::MonsterKind> selected)
    : journal_(journal), open_(std::move(open)) {
  refresh();
  if (selected) {
    const auto found = std::ranges::find(monsters_, *selected);
    if (found != monsters_.end()) {
      selected_ = static_cast<int>(found - monsters_.begin());
    }
  }
  refresh_skills();
  auto monster_options = ftxui::MenuOption{};
  monster_options.on_change = [this] {
    selected_skill_ = 0;
    refresh_skills();
  };
  monster_menu_ = ftxui::Menu(&names_, &selected_, monster_options);
  auto skill_options = ftxui::MenuOption{};
  skill_options.on_enter = [this] { open_skill(); };
  skill_menu_ = ftxui::Menu(&skill_names_, &selected_skill_, skill_options);
  Add(ftxui::Container::Horizontal({monster_menu_, skill_menu_}));
}

void BestiaryView::refresh() {
  const auto previous =
      monsters_.empty()
          ? std::nullopt
          : std::optional{monsters_[static_cast<std::size_t>(selected_)]};
  monsters_.clear();
  names_.clear();
  for (const auto &[kind, skills] : journal_.monsters()) {
    (void)skills;
    monsters_.push_back(kind);
  }
  std::ranges::sort(monsters_, [](auto lhs, auto rhs) {
    return fl::monster::generated_content::stats(lhs).display_name <
           fl::monster::generated_content::stats(rhs).display_name;
  });
  selected_ = 0;
  for (auto kind : monsters_) {
    if (previous == kind) {
      selected_ = static_cast<int>(names_.size());
    }
    names_.emplace_back(
        fl::monster::generated_content::stats(kind).display_name);
  }
  refresh_skills();
}

void BestiaryView::refresh_skills() {
  skills_ = monsters_.empty()
                ? std::vector<std::optional<fl::skills::SkillId>>{}
                : journal_.skill_slots(
                      monsters_[static_cast<std::size_t>(selected_)]);
  skill_names_.clear();
  for (const auto skill : skills_) {
    skill_names_.push_back(skill ? fl::skills::display_name(*skill) + " >"
                                 : "--");
  }
  selected_skill_ = std::clamp(
      selected_skill_, 0, std::max(0, static_cast<int>(skills_.size()) - 1));
}

void BestiaryView::open_skill() {
  refresh_skills();
  if (!skills_.empty() && skills_[static_cast<std::size_t>(selected_skill_)] &&
      open_) {
    open_(monsters_[static_cast<std::size_t>(selected_)],
          *skills_[static_cast<std::size_t>(selected_skill_)]);
  }
}

ftxui::Element BestiaryView::Render() {
  using namespace ftxui;
  refresh();
  if (monsters_.empty()) {
    return window(text("Bestiary"), paragraph("No monsters encountered yet."));
  }
  const auto stats = fl::monster::generated_content::stats(
      monsters_[static_cast<std::size_t>(selected_)]);
  return window(
      text("Bestiary"),
      vbox({hbox({monster_menu_->Render() | vscroll_indicator | yframe |
                      size(WIDTH, LESS_THAN, 30),
                  separator(),
                  vbox({text(std::string{stats.display_name}) | bold,
                        text("Base HP " + std::to_string(stats.hp) + "   MP " +
                             std::to_string(stats.mp)),
                        separator(), text("Witnessed skills"),
                        skill_menu_->Render() | vscroll_indicator | yframe |
                            flex}) |
                      flex}) |
                flex,
            separator(),
            paragraph("Tab/Left/Right: monsters or skills   Up/Down: select   "
                      "Enter: open skill   --: unwitnessed   Esc: back") |
                dim}));
}

} // namespace fl::widgets
