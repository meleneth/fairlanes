#include "fl/widgets/libram_view.hpp"

#include "fl/skills/skill_definition.hpp"
#include <algorithm>

namespace fl::widgets {

LibramView::LibramView(const fl::primitives::DiscoveryJournal &journal,
                       std::optional<fl::skills::SkillId> selected)
    : journal_(journal) {
  refresh();
  if (selected) {
    const auto found = std::ranges::find(skills_, *selected);
    if (found != skills_.end()) {
      selected_ = static_cast<int>(found - skills_.begin());
    }
  }
  menu_ = ftxui::Menu(&names_, &selected_);
  Add(menu_);
}

void LibramView::refresh() {
  const auto previous =
      skills_.empty()
          ? std::nullopt
          : std::optional{skills_[static_cast<std::size_t>(selected_)]};
  skills_.clear();
  names_.clear();
  for (const auto *entry : fl::skills::all_definitions()) {
    if (journal_.knows(entry->key.base)) {
      skills_.push_back(entry->key.base);
    }
  }
  std::ranges::sort(skills_, [](auto lhs, auto rhs) {
    return fl::skills::name(lhs) < fl::skills::name(rhs);
  });
  selected_ = 0;
  for (auto skill : skills_) {
    if (previous == skill) {
      selected_ = static_cast<int>(names_.size());
    }
    names_.emplace_back(fl::skills::name(skill));
  }
}

ftxui::Element LibramView::Render() {
  using namespace ftxui;
  refresh();
  if (skills_.empty()) {
    return window(text("Libram"),
                  paragraph("No skills witnessed yet. Watch combat to reveal "
                            "their descriptions."));
  }
  const auto &entry =
      fl::skills::definition(skills_[static_cast<std::size_t>(selected_)]);
  return window(text("Libram"),
                vbox({hbox({menu_->Render() | vscroll_indicator | yframe |
                                size(WIDTH, LESS_THAN, 30),
                            separator(),
                            vbox({text(std::string{entry.display_name}) | bold,
                                  separator(),
                                  paragraph(std::string{entry.description})}) |
                                flex}) |
                          flex,
                      separator(),
                      text("Up/Down: skill   Esc: back   b: bestiary") | dim}));
}

} // namespace fl::widgets
