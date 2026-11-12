#include "fl/widgets/libram_view.hpp"

#include "fl/skills/skill_definition.hpp"
#include "fl/skills/skill_scaling.hpp"
#include <algorithm>

namespace fl::widgets {

LibramView::LibramView(const fl::primitives::DiscoveryJournal& journal,
                       std::optional<fl::skills::SkillId> selected,
                       fl::primitives::PartyData* party)
    : journal_(journal), party_(party) {
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

bool LibramView::OnEvent(ftxui::Event event) {
  if (party_ && !party_->members().empty() &&
      (event == ftxui::Event::ArrowLeft || event == ftxui::Event::ArrowRight)) {
    const int count = static_cast<int>(party_->members().size());
    member_ =
        event == ftxui::Event::ArrowRight ? (member_ + 1) % count : (member_ + count - 1) % count;
    action_status_.clear();
    return true;
  }
  if (party_ && !party_->members().empty() && !skills_.empty() &&
      event == ftxui::Event::Character("f")) {
    member_ = std::clamp(member_, 0, static_cast<int>(party_->members().size()) - 1);
    const auto member = party_->members()[static_cast<std::size_t>(member_)].member_id();
    const fl::skills::SkillKey skill{skills_[static_cast<std::size_t>(selected_)]};
    action_status_ = party_->farm_for_forgotten_skill(member, skill)
                         ? "Farming request accepted for the next encounter boundary."
                         : "No accessible teacher for this character and party.";
    return true;
  }
  return ComponentBase::OnEvent(event);
}

void LibramView::refresh() {
  const auto previous =
      skills_.empty() ? std::nullopt : std::optional{skills_[static_cast<std::size_t>(selected_)]};
  skills_.clear();
  names_.clear();
  for (const auto* entry : fl::skills::all_definitions()) {
    if (journal_.knows(entry->key.base)) {
      skills_.push_back(entry->key.base);
    }
  }
  std::ranges::sort(
      skills_, [](auto lhs, auto rhs) { return fl::skills::name(lhs) < fl::skills::name(rhs); });
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
    return window(text("Libram"), paragraph("No skills witnessed yet. Watch combat to reveal "
                                            "their descriptions."));
  }
  const auto& entry = fl::skills::definition(skills_[static_cast<std::size_t>(selected_)]);
  Elements details{text(std::string{entry.display_name}) | bold, separator(),
                   paragraph(std::string{entry.description}),
                   paragraph("Scaling [" +
                             std::string(fl::skills::to_string(entry.scaling.dimension)) +
                             "]: " + fl::skills::describe_scaling(entry.scaling))};
  if (party_ && !party_->members().empty()) {
    member_ = std::clamp(member_, 0, static_cast<int>(party_->members().size()) - 1);
    const auto& member = party_->members()[static_cast<std::size_t>(member_)];
    const fl::skills::SkillKey skill{entry.key.base};
    const auto& grimoire = member.grimoire();
    const std::string state = grimoire.knows(skill)       ? "known"
                              : grimoire.forgotten(skill) ? "forgotten"
                                                          : "not learned";
    details.push_back(separator());
    details.push_back(text(member.name() + ": " + state));
    const auto current = fl::skills::evaluate_scaling(party_->party_ctx().reg(), member.member_id(),
                                                      entry.scaling, entry.scaling.base_effect);
    details.push_back(text("Current " +
                           std::string(fl::skills::to_string(entry.scaling.dimension)) + ": " +
                           std::to_string(current.total)));
    if (grimoire.forgotten(skill)) details.push_back(text("f: Farm for this skill"));
    if (!action_status_.empty()) details.push_back(paragraph(action_status_));
  }
  return window(
      text("Libram"),
      vbox({hbox({menu_->Render() | vscroll_indicator | yframe | size(WIDTH, LESS_THAN, 30),
                  separator(), vbox(std::move(details)) | flex}) |
                flex,
            separator(),
            paragraph("No balance passes have been done. Damage numbers "
                      "are base values before misses, armor, shields "
                      "and field modifiers. Times use combat seconds."),
            text("Up/Down: skill   Left/Right: character   f: farm forgotten   Esc: back   b: "
                 "bestiary") |
                dim}));
}

} // namespace fl::widgets
