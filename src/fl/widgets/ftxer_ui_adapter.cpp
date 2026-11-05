#include "ftxer_ui_adapter.hpp"

#include <stdexcept>
#include <string_view>

#include "root_component.hpp"

namespace fl::widgets {

namespace generated = fl::generated::ftxer;

FtxerUiAdapter::FtxerUiAdapter(ftxui::Component root) : root_(std::move(root)) {
  if (!dynamic_cast<RootComponent*>(root_.get()))
    throw std::invalid_argument("FtxerUiAdapter requires RootComponent");
}

ftxui::Component FtxerUiAdapter::component() {
  return root_;
}
ftxui::Element FtxerUiAdapter::render() {
  return root().Render();
}
bool FtxerUiAdapter::on_event(const ftxui::Event& event) {
  return root().OnEvent(event);
}

generated::ScreenId FtxerUiAdapter::current_screen() const {
  const auto value = root().active_screen_id();
  if (value == "raid") return generated::ScreenId::Raid;
  if (value == "party") return generated::ScreenId::Party;
  if (value == "account_battle") return generated::ScreenId::AccountBattle;
  if (value == "party_battle") return generated::ScreenId::PartyBattle;
  if (value == "effect_gallery") return generated::ScreenId::EffectGallery;
  if (value == "libram") return generated::ScreenId::Libram;
  if (value == "bestiary") return generated::ScreenId::Bestiary;
  return generated::ScreenId::Story;
}

bool FtxerUiAdapter::show_screen(generated::ScreenId screen) {
  switch (screen) {
  case generated::ScreenId::Raid: return root().show_generated_screen("raid");
  case generated::ScreenId::Party: return root().show_generated_screen("party");
  case generated::ScreenId::AccountBattle: return root().show_generated_screen("account_battle");
  case generated::ScreenId::PartyBattle: return root().show_generated_screen("party_battle");
  case generated::ScreenId::EffectGallery: return root().show_generated_screen("effect_gallery");
  case generated::ScreenId::Libram: return root().show_generated_screen("libram");
  case generated::ScreenId::Bestiary: return root().show_generated_screen("bestiary");
  case generated::ScreenId::Story: return root().show_generated_screen("story");
  }
  return false;
}

bool FtxerUiAdapter::invoke(generated::ExternalAction action) {
  switch (action) {
  case generated::ExternalAction::CollapseHelp:
    return root().invoke_generated_action("collapse_help");
  case generated::ExternalAction::ExpandHelp: return root().invoke_generated_action("expand_help");
  case generated::ExternalAction::NextAccount:
    return root().invoke_generated_action("next_account");
  case generated::ExternalAction::NextParty: return root().invoke_generated_action("next_party");
  case generated::ExternalAction::OverdriveDown:
    return root().invoke_generated_action("overdrive_down");
  case generated::ExternalAction::OverdriveUp:
    return root().invoke_generated_action("overdrive_up");
  case generated::ExternalAction::PreviousAccount:
    return root().invoke_generated_action("previous_account");
  case generated::ExternalAction::PreviousParty:
    return root().invoke_generated_action("previous_party");
  case generated::ExternalAction::ReturnToTown:
    return root().invoke_generated_action("return_to_town");
  case generated::ExternalAction::ShowObjectives:
    return root().invoke_generated_action("show_objectives");
  case generated::ExternalAction::ShowVillage:
    return root().invoke_generated_action("show_village");
  case generated::ExternalAction::ToggleActiveScreen:
    return root().invoke_generated_action("toggle_active_screen");
  case generated::ExternalAction::ToggleAutoplay:
    return root().invoke_generated_action("toggle_autoplay");
  case generated::ExternalAction::ToggleHelp: return root().invoke_generated_action("toggle_help");
  case generated::ExternalAction::TogglePartyBattle:
    return root().invoke_generated_action("toggle_party_battle");
  case generated::ExternalAction::BackOrQuit: return root().invoke_generated_action("back_or_quit");
  }
  return false;
}

bool FtxerUiAdapter::console_open() const {
  return root().console_open();
}
void FtxerUiAdapter::toggle_console() {
  root().toggle_console();
}
void FtxerUiAdapter::focus_console() {
  root().focus_console();
}
void FtxerUiAdapter::restore_focus() {
  root().restore_active_focus();
}
bool FtxerUiAdapter::quit_requested() const {
  return root().quit_requested();
}
bool FtxerUiAdapter::sandbox_requested() const {
  return root().sandbox_requested();
}

RootComponent& FtxerUiAdapter::root() {
  return *dynamic_cast<RootComponent*>(root_.get());
}
const RootComponent& FtxerUiAdapter::root() const {
  return *dynamic_cast<const RootComponent*>(root_.get());
}

} // namespace fl::widgets
