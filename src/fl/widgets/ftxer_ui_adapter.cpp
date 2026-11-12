#include "ftxer_ui_adapter.hpp"

#include <stdexcept>
#include <utility>

namespace fl::widgets {

namespace generated = fl::generated::ftxer;

FtxerUiAdapter::FtxerUiAdapter(std::unique_ptr<RootComponent> root) : root_(std::move(root)) {
  if (!root_) throw std::invalid_argument("FtxerUiAdapter requires RootComponent");
}

void FtxerUiAdapter::bind_shell(generated::ShellCallbacks callbacks) {
  root_->bind_shell(std::move(callbacks));
}
generated::ScreenId FtxerUiAdapter::initial_screen(generated::ScreenId declared) const {
  return root_->initial_screen(declared);
}
ftxui::Component FtxerUiAdapter::makeRaid() {
  return root_->make_raid();
}
ftxui::Component FtxerUiAdapter::makeParty() {
  return root_->make_party();
}
ftxui::Component FtxerUiAdapter::makeAccountBattle() {
  return root_->make_account_battle();
}
ftxui::Component FtxerUiAdapter::makePartyBattle() {
  return root_->make_party_battle();
}
ftxui::Component FtxerUiAdapter::makeEffectGallery() {
  return root_->make_effect_gallery();
}
ftxui::Component FtxerUiAdapter::makeLibram() {
  return root_->make_libram();
}
ftxui::Component FtxerUiAdapter::makeBestiary() {
  return root_->make_bestiary();
}
ftxui::Component FtxerUiAdapter::makeObjectives() {
  return root_->make_objectives();
}
ftxui::Component FtxerUiAdapter::makeVillage() {
  return root_->make_village();
}
ftxui::Component FtxerUiAdapter::makeConsoleOverlay() {
  return root_->make_console_overlay();
}
ftxui::Component FtxerUiAdapter::makeQuitConfirmationOverlay() {
  return root_->make_quit_confirmation_overlay();
}
ftxui::Component FtxerUiAdapter::makeAutoplayConfirmationOverlay() {
  return root_->make_autoplay_confirmation_overlay();
}
ftxui::Component FtxerUiAdapter::makeSandboxConfirmationOverlay() {
  return root_->make_sandbox_confirmation_overlay();
}
ftxui::Component FtxerUiAdapter::makeKeybindHelpOverlay() {
  return root_->make_keybind_help_overlay();
}
ftxui::Component FtxerUiAdapter::makeCharacterManagementOverlay() {
  return root_->make_character_management_overlay();
}
ftxui::Element FtxerUiAdapter::decorate(generated::ScreenId screen, ftxui::Element content) {
  return root_->decorate(screen, std::move(content));
}
bool FtxerUiAdapter::intercept(const ftxui::Event& event) {
  return root_->intercept(event);
}
void FtxerUiAdapter::overlay_opened(generated::OverlayId overlay) {
  root_->overlay_opened(overlay);
}
void FtxerUiAdapter::overlay_closed(generated::OverlayId overlay) {
  root_->overlay_closed(overlay);
}
bool FtxerUiAdapter::collapse_help(generated::ScreenId current) {
  return root_->collapse_help(current);
}
bool FtxerUiAdapter::expand_help(generated::ScreenId current) {
  return root_->expand_help(current);
}
bool FtxerUiAdapter::next_account(generated::ScreenId current) {
  return root_->next_account(current);
}
bool FtxerUiAdapter::next_party(generated::ScreenId current) {
  return root_->next_party(current);
}
bool FtxerUiAdapter::overdrive_down(generated::ScreenId current) {
  return root_->overdrive_down(current);
}
bool FtxerUiAdapter::overdrive_up(generated::ScreenId current) {
  return root_->overdrive_up(current);
}
bool FtxerUiAdapter::previous_account(generated::ScreenId current) {
  return root_->previous_account(current);
}
bool FtxerUiAdapter::previous_party(generated::ScreenId current) {
  return root_->previous_party(current);
}
bool FtxerUiAdapter::return_to_town(generated::ScreenId current) {
  return root_->return_to_town(current);
}
bool FtxerUiAdapter::toggle_autoplay(generated::ScreenId current) {
  return root_->toggle_autoplay(current);
}
bool FtxerUiAdapter::toggle_help(generated::ScreenId current) {
  return root_->toggle_help(current);
}
bool FtxerUiAdapter::open_character_management(generated::ScreenId current) {
  return root_->open_character_management(current);
}
bool FtxerUiAdapter::quit_requested() const {
  return root_->quit_requested();
}
bool FtxerUiAdapter::sandbox_requested() const {
  return root_->sandbox_requested();
}

} // namespace fl::widgets
