#include "fl/generated/ftxer_composition_root.hpp"

#include <algorithm>
#include <utility>

namespace fl::generated::ftxer {

CompositionRoot::CompositionRoot(std::unique_ptr<ExternalUiAdapter> adapter)
    : adapter_(std::move(adapter)),
      console_overlay_(adapter_->makeConsoleOverlay()),
      quit_confirmation_overlay_(adapter_->makeQuitConfirmationOverlay()),
      autoplay_confirmation_overlay_(adapter_->makeAutoplayConfirmationOverlay()),
      sandbox_confirmation_overlay_(adapter_->makeSandboxConfirmationOverlay()),
      keybind_help_overlay_(adapter_->makeKeybindHelpOverlay()) {
  adapter_->bind_shell({
      [this](ScreenId screen, bool remember) { return navigate(screen, remember); },
      [this](OverlayId overlay) { open_overlay(overlay); },
      [this](OverlayId overlay) { close_overlay(overlay); },
      [this] { refresh(); },
  });
  Add(console_overlay_);
  Add(quit_confirmation_overlay_);
  Add(autoplay_confirmation_overlay_);
  Add(sandbox_confirmation_overlay_);
  Add(keybind_help_overlay_);
  current_screen_ = adapter_->initial_screen(current_screen_);
  active_screen_ = make_screen(current_screen_);
  Add(active_screen_);
  active_screen_->TakeFocus();
}

ftxui::Component CompositionRoot::ActiveChild() {
  return overlays_.empty() ? active_screen_ : overlay_component(overlays_.back());
}

bool CompositionRoot::console_open() const {
  return std::find(overlays_.begin(), overlays_.end(), OverlayId::Console) != overlays_.end();
}

bool CompositionRoot::OnEvent(ftxui::Event event) {
  if (event == ftxui::Event::Character("`")) {
    if (console_open()) close_overlay(OverlayId::Console);
    else open_overlay(OverlayId::Console);
    return true;
  }
  if (!overlays_.empty()) {
    const auto overlay = overlays_.back();
    if (event == ftxui::Event::Escape) {
      close_overlay(overlay);
      return true;
    }
    return overlay_component(overlay)->OnEvent(event);
  }
  if (adapter_->intercept(event)) return true;
  if (event == ftxui::Event::Escape && (current_screen_ == ScreenId::Party || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village) && active_screen_->OnEvent(event)) return true;
  if (event == ftxui::Event::Character("q") && (current_screen_ == ScreenId::Party || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village) && active_screen_->OnEvent(event)) return true;
  if (run_binding(event)) return true;
  return active_screen_ && active_screen_->OnEvent(event);
}

ftxui::Element CompositionRoot::Render() {
  auto content = adapter_->decorate(current_screen_, active_screen_->Render());
  for (const auto overlay : overlays_)
    content = ftxui::dbox({std::move(content), overlay_component(overlay)->Render()});
  return content;
}

bool CompositionRoot::run_binding(const ftxui::Event& event) {
  if (event == ftxui::Event::Character("l") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return navigate(ScreenId::Libram);
  if (event == ftxui::Event::Character("b") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return navigate(ScreenId::Bestiary);
  if (event == ftxui::Event::Character("o") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return navigate(ScreenId::Objectives);
  if (event == ftxui::Event::Character("v") && (current_screen_ == ScreenId::Party || current_screen_ == ScreenId::PartyBattle)) return navigate(ScreenId::Village);
  if (event == ftxui::Event::Character("v") && ((current_screen_ == ScreenId::Objectives) && !history_.empty() && (history_.back() == ScreenId::Party || history_.back() == ScreenId::PartyBattle))) return navigate(ScreenId::Village);
  if (event == ftxui::Event::Character("a") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->toggle_autoplay(current_screen_);
    return consumed; }());
  if (event == ftxui::Event::Character("h") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->toggle_help(current_screen_);
    return consumed; }());
  if (event == ftxui::Event::Character("r") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->return_to_town(current_screen_);
    return consumed; }());
  if (event == ftxui::Event::Character(" ") && (current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return navigate(ScreenId::PartyBattle);
  if (event == ftxui::Event::Character(" ") && (current_screen_ == ScreenId::PartyBattle)) return navigate(ScreenId::AccountBattle);
  if (event == ftxui::Event::Character("/") && (current_screen_ == ScreenId::Party)) return navigate(ScreenId::AccountBattle);
  if (event == ftxui::Event::Character("/") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle)) return navigate(ScreenId::Party);
  if (event == ftxui::Event::Character("/") && (current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return navigate(ScreenId::PartyBattle);
  if (event == ftxui::Event::Character(">") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->collapse_help(current_screen_);
    return consumed; }());
  if (event == ftxui::Event::Character(".") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->collapse_help(current_screen_);
    return consumed; }());
  if (event == ftxui::Event::Character("<") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->expand_help(current_screen_);
    return consumed; }());
  if (event == ftxui::Event::Character(",") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->expand_help(current_screen_);
    return consumed; }());
  if (event == ftxui::Event::Character("+") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->overdrive_up(current_screen_);
    return consumed; }());
  if (event == ftxui::Event::Character("=") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->overdrive_up(current_screen_);
    return consumed; }());
  if (event == ftxui::Event::Character("-") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->overdrive_down(current_screen_);
    return consumed; }());
  if (event == ftxui::Event::Character("[") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->previous_party(current_screen_);
    if (consumed) refresh();
    return consumed; }());
  if (event == ftxui::Event::Character("]") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->next_party(current_screen_);
    if (consumed) refresh();
    return consumed; }());
  if (event == ftxui::Event::Character("{") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->previous_account(current_screen_);
    if (consumed) refresh();
    return consumed; }());
  if (event == ftxui::Event::Character(";") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->previous_account(current_screen_);
    if (consumed) refresh();
    return consumed; }());
  if (event == ftxui::Event::Character("}") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->next_account(current_screen_);
    if (consumed) refresh();
    return consumed; }());
  if (event == ftxui::Event::Character("'") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::Party || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return ([&] { const bool consumed = adapter_->next_account(current_screen_);
    if (consumed) refresh();
    return consumed; }());
  if (event == ftxui::Event::Return && (current_screen_ == ScreenId::Raid)) return navigate(ScreenId::PartyBattle);
  if (event == ftxui::Event::Escape && (current_screen_ == ScreenId::Party || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return back();
  if (event == ftxui::Event::Escape && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary)) return back();
  if (event == ftxui::Event::Character("q") && (current_screen_ == ScreenId::Party || current_screen_ == ScreenId::Objectives || current_screen_ == ScreenId::Village)) return back();
  if (event == ftxui::Event::Character("q") && (current_screen_ == ScreenId::Raid || current_screen_ == ScreenId::AccountBattle || current_screen_ == ScreenId::PartyBattle || current_screen_ == ScreenId::EffectGallery || current_screen_ == ScreenId::Libram || current_screen_ == ScreenId::Bestiary)) return back();
  return false;
}

bool CompositionRoot::navigate(ScreenId destination, bool remember, bool refresh_screen) {
  if (destination == current_screen_ && !refresh_screen) return true;
  auto next = make_screen(destination);
  if (!next) return false;
  if (remember && destination != current_screen_) history_.push_back(current_screen_);
  active_screen_->Detach();
  active_screen_ = std::move(next);
  Add(active_screen_);
  current_screen_ = destination;
  if (overlays_.empty()) active_screen_->TakeFocus();
  return true;
}

void CompositionRoot::refresh() { navigate(current_screen_, false, true); }

bool CompositionRoot::back() {
  if (history_.empty()) {
    open_overlay(OverlayId::QuitConfirmation);
  return true;
  }
  const auto destination = history_.back();
  history_.pop_back();
  return navigate(destination, false);
}

void CompositionRoot::open_overlay(OverlayId overlay) {
  const auto found = std::find(overlays_.begin(), overlays_.end(), overlay);
  if (found != overlays_.end()) overlays_.erase(found);
  overlays_.push_back(overlay);
  adapter_->overlay_opened(overlay);
  overlay_component(overlay)->TakeFocus();
}

void CompositionRoot::close_overlay(OverlayId overlay) {
  const auto found = std::find(overlays_.begin(), overlays_.end(), overlay);
  if (found == overlays_.end()) return;
  const bool was_top = found == overlays_.end() - 1;
  overlays_.erase(found);
  adapter_->overlay_closed(overlay);
  if (!was_top) return;
  if (overlays_.empty()) active_screen_->TakeFocus();
  else overlay_component(overlays_.back())->TakeFocus();
}

ftxui::Component CompositionRoot::make_screen(ScreenId screen) {
  switch (screen) {
  case ScreenId::Raid: return adapter_->makeRaid();
  case ScreenId::Party: return adapter_->makeParty();
  case ScreenId::AccountBattle: return adapter_->makeAccountBattle();
  case ScreenId::PartyBattle: return adapter_->makePartyBattle();
  case ScreenId::EffectGallery: return adapter_->makeEffectGallery();
  case ScreenId::Libram: return adapter_->makeLibram();
  case ScreenId::Bestiary: return adapter_->makeBestiary();
  case ScreenId::Objectives: return adapter_->makeObjectives();
  case ScreenId::Village: return adapter_->makeVillage();
  }
  return {};
}

ftxui::Component CompositionRoot::overlay_component(OverlayId overlay) const {
  switch (overlay) {
  case OverlayId::Console: return console_overlay_;
  case OverlayId::QuitConfirmation: return quit_confirmation_overlay_;
  case OverlayId::AutoplayConfirmation: return autoplay_confirmation_overlay_;
  case OverlayId::SandboxConfirmation: return sandbox_confirmation_overlay_;
  case OverlayId::KeybindHelp: return keybind_help_overlay_;
  }
  return {};
}

}  // namespace fl::generated::ftxer
