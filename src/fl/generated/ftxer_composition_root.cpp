#include "fl/generated/ftxer_composition_root.hpp"

namespace fl::generated::ftxer {

CompositionRoot::CompositionRoot(std::unique_ptr<ExternalUiAdapter> adapter)
    : adapter_(std::move(adapter)), external_component_(adapter_->component()),
      current_screen_(adapter_->current_screen()) {
  Add(external_component_);
  dispatch(current_screen_);
}

ftxui::Component CompositionRoot::ActiveChild() { return external_component_; }

bool CompositionRoot::OnEvent(ftxui::Event event) {
  // Reserved shell toggle: no external widget, modal, or focused input sees it.
  if (event == ftxui::Event::Character("`")) {
    const bool was_open = adapter_->console_open();
    adapter_->toggle_console();
    if (!was_open && adapter_->console_open()) adapter_->focus_console();
    if (was_open && !adapter_->console_open()) adapter_->restore_focus();
    return true;
  }
  if (adapter_->console_open()) {
    const bool consumed = adapter_->on_event(event);
    synchronize(true);
    return consumed;
  }
  if (event == ftxui::Event::Character("l")) return navigate(ScreenId::Libram);
  if (event == ftxui::Event::Character("b")) return navigate(ScreenId::Bestiary);
  if (event == ftxui::Event::Character("o")) return invoke(ExternalAction::ShowObjectives);
  if (event == ftxui::Event::Character("v")) return invoke(ExternalAction::ShowVillage);
  if (event == ftxui::Event::Character("a")) return invoke(ExternalAction::ToggleAutoplay);
  if (event == ftxui::Event::Character("h")) return invoke(ExternalAction::ToggleHelp);
  if (event == ftxui::Event::Character("r")) return invoke(ExternalAction::ReturnToTown);
  if (event == ftxui::Event::Character(" ")) return invoke(ExternalAction::TogglePartyBattle);
  if (event == ftxui::Event::Character("/")) return invoke(ExternalAction::ToggleActiveScreen);
  if (event == ftxui::Event::Character(">")) return invoke(ExternalAction::CollapseHelp);
  if (event == ftxui::Event::Character(".")) return invoke(ExternalAction::CollapseHelp);
  if (event == ftxui::Event::Character("<")) return invoke(ExternalAction::ExpandHelp);
  if (event == ftxui::Event::Character(",")) return invoke(ExternalAction::ExpandHelp);
  if (event == ftxui::Event::Character("+")) return invoke(ExternalAction::OverdriveUp);
  if (event == ftxui::Event::Character("=")) return invoke(ExternalAction::OverdriveUp);
  if (event == ftxui::Event::Character("-")) return invoke(ExternalAction::OverdriveDown);
  if (event == ftxui::Event::Character("[")) return invoke(ExternalAction::PreviousParty);
  if (event == ftxui::Event::Character("]")) return invoke(ExternalAction::NextParty);
  if (event == ftxui::Event::Character("{")) return invoke(ExternalAction::PreviousAccount);
  if (event == ftxui::Event::Character(";")) return invoke(ExternalAction::PreviousAccount);
  if (event == ftxui::Event::Character("}")) return invoke(ExternalAction::NextAccount);
  if (event == ftxui::Event::Character("'")) return invoke(ExternalAction::NextAccount);
  if (event == ftxui::Event::Escape) return back();
  if (event == ftxui::Event::Character("q")) return back();
  const bool consumed = adapter_->on_event(event);
  synchronize(true);
  return consumed;
}

ftxui::Element CompositionRoot::Render() {
  synchronize(false);
  return adapter_->render();
}

bool CompositionRoot::navigate(ScreenId destination, bool remember) {
  if (destination == current_screen_) return true;
  const auto previous = current_screen_;
  if (!adapter_->show_screen(destination)) return false;
  if (remember) history_.push_back(previous);
  dispatch(destination);
  return true;
}

bool CompositionRoot::back() {
  if (history_.empty()) return adapter_->invoke(ExternalAction::BackOrQuit);
  const auto destination = history_.back();
  history_.pop_back();
  return navigate(destination, false);
}

bool CompositionRoot::invoke(ExternalAction action) {
  const auto previous = adapter_->current_screen();
  const bool consumed = adapter_->invoke(action);
  const auto current = adapter_->current_screen();
  if (current != previous) {
    history_.push_back(previous);
    dispatch(current);
  }
  return consumed;
}

void CompositionRoot::synchronize(bool remember) {
  const auto actual = adapter_->current_screen();
  if (actual == current_screen_) return;
  if (remember) history_.push_back(current_screen_);
  dispatch(actual);
}

void CompositionRoot::dispatch(ScreenId destination) {
  switch (destination) {
  case ScreenId::Raid: navigation_.process_event(GoToRaid{}); break;
  case ScreenId::Party: navigation_.process_event(GoToParty{}); break;
  case ScreenId::AccountBattle: navigation_.process_event(GoToAccountBattle{}); break;
  case ScreenId::PartyBattle: navigation_.process_event(GoToPartyBattle{}); break;
  case ScreenId::EffectGallery: navigation_.process_event(GoToEffectGallery{}); break;
  case ScreenId::Libram: navigation_.process_event(GoToLibram{}); break;
  case ScreenId::Bestiary: navigation_.process_event(GoToBestiary{}); break;
  case ScreenId::Story: navigation_.process_event(GoToStory{}); break;
  }
  current_screen_ = destination;
}

}  // namespace fl::generated::ftxer
