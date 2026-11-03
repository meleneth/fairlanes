#include "story_screen.hpp"

#include <string>
#include <utility>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "fl/generated/story_content.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/objective_system.hpp"

namespace fl::widgets {

StoryScreen::StoryScreen(const fl::primitives::ObjectiveSystem &objectives,
                         EmitChoice emit_choice, Purchase purchase, Start start)
    : objectives_(&objectives), emit_choice_(std::move(emit_choice)),
      purchase_(std::move(purchase)),
      mode_(start == Start::Objectives ? Mode::Objectives : Mode::Village) {}

void StoryScreen::open_dialogue(std::string_view id) {
  const auto *dialogue = fl::story::generated_content::find_dialogue(id);
  if (!dialogue || !objectives_->can_talk(id)) return;
  dialogue_id_ = id;
  node_id_ = dialogue->start_node;
  selection_ = 0;
  mode_ = Mode::Dialogue;
}

bool StoryScreen::OnEvent(ftxui::Event event) {
  if (event == ftxui::Event::Escape || event == ftxui::Event::Character("q")) {
    if (mode_ == Mode::Dialogue) mode_ = dialogue_id_ == "angry_fellow" ? Mode::Pub : Mode::Village;
    else if (mode_ == Mode::Pub) mode_ = Mode::Village;
    else return false;
    selection_ = 0;
    return true;
  }
  if (mode_ == Mode::Objectives) return false;
  const auto *dialogue = fl::story::generated_content::find_dialogue(dialogue_id_);
  const auto *node = dialogue && mode_ == Mode::Dialogue
                         ? fl::story::generated_content::find_node(*dialogue, node_id_)
                         : nullptr;
  const std::size_t count = mode_ == Mode::Dialogue && node
                                ? node->choices.size()
                                : mode_ == Mode::Village && objectives_->can_talk("second_volunteer") ? 3
                                : mode_ == Mode::Village ? 2 : 1;
  if (event == ftxui::Event::ArrowDown || event == ftxui::Event::Character("j")) {
    if (count) selection_ = (selection_ + 1) % count;
    return true;
  }
  if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character("k")) {
    if (count) selection_ = (selection_ + count - 1) % count;
    return true;
  }
  if (event != ftxui::Event::Return) return false;
  if (mode_ == Mode::Village) {
    if (selection_ == 0) mode_ = Mode::Pub;
    else if (selection_ == 1) purchase_();
    else open_dialogue("second_volunteer");
  } else if (mode_ == Mode::Pub) {
    open_dialogue("angry_fellow");
  } else if (node && selection_ < node->choices.size()) {
    const auto &choice = node->choices[selection_];
    if (!choice.event_id.empty()) emit_choice_(choice.event_id);
    if (choice.next_node == "exit") mode_ = Mode::Village;
    else node_id_ = choice.next_node;
  } else if (node && node->choices.empty()) {
    mode_ = Mode::Village;
  }
  selection_ = 0;
  return true;
}

ftxui::Element StoryScreen::Render() {
  using namespace ftxui;
  std::vector<Element> lines;
  std::string title;
  if (mode_ == Mode::Objectives) {
    title = "Objectives";
    bool any = false;
    for (const auto &objective : fl::story::generated_content::kObjectives) {
      if (!objectives_->active(objective.id) && !objectives_->completed(objective.id)) continue;
      any = true;
      lines.push_back(text(std::string(objectives_->completed(objective.id) ? "[done] " : "[active] ") + std::string(objective.title)) | bold);
      lines.push_back(paragraph(std::string(objective.description)));
      lines.push_back(separator());
    }
    if (!any) lines.push_back(text("No objectives revealed yet."));
  } else if (mode_ == Mode::Village) {
    title = "Village";
    lines.push_back(paragraph("The first Visitor left the village in ruins. The pub survived."));
    lines.push_back(text(std::string(selection_ == 0 ? "> " : "  ") + "Visit the pub"));
    lines.push_back(text(std::string(selection_ == 1 ? "> " : "  ") +
                         "Buy Village Guard Hide (10 Darrin-marks)" +
                         (objectives_->can_buy_village_guard_hide() ? "" : " [unavailable]")));
    lines.push_back(text("Purse: " + std::to_string(objectives_->darrin_marks()) + " Darrin-marks"));
    if (objectives_->can_talk("second_volunteer"))
      lines.push_back(text(std::string(selection_ == 2 ? "> " : "  ") + "Speak to the volunteer"));
  } else if (mode_ == Mode::Pub) {
    title = "Village Pub";
    lines.push_back(paragraph("An angry-looking fellow watches the door."));
    lines.push_back(text(objectives_->can_talk("angry_fellow")
                             ? "> Speak to the angry fellow"
                             : "He has already joined your party."));
  } else {
    const auto *dialogue = fl::story::generated_content::find_dialogue(dialogue_id_);
    const auto *node = dialogue ? fl::story::generated_content::find_node(*dialogue, node_id_) : nullptr;
    title = dialogue ? std::string(dialogue->npc) : "Dialogue";
    if (node) {
      lines.push_back(text(std::string(node->speaker)) | bold);
      lines.push_back(paragraph(std::string(node->text)));
      lines.push_back(separator());
      for (std::size_t i = 0; i < node->choices.size(); ++i)
        lines.push_back(text(std::string(i == selection_ ? "> " : "  ") + std::string(node->choices[i].label)));
      if (node->choices.empty()) lines.push_back(text("Enter to continue"));
    }
  }
  lines.push_back(separator());
  lines.push_back(text("Up/Down choose  Enter confirm  Esc back") | dim);
  return window(text(title) | bold, vbox(std::move(lines))) |
         bgcolor(fl::lospec500::color_at(0)) |
         color(fl::lospec500::color_at(32)) | flex;
}

} // namespace fl::widgets
