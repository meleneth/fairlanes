#pragma once

#include <memory>

#include "fl/generated/ftxer_composition_root.hpp"
#include "fl/widgets/root_component.hpp"

namespace fl::widgets {

class FtxerUiAdapter final : public fl::generated::ftxer::ExternalUiAdapter {
public:
  explicit FtxerUiAdapter(std::unique_ptr<RootComponent> root);

  void bind_shell(fl::generated::ftxer::ShellCallbacks callbacks) override;
  fl::generated::ftxer::ScreenId
  initial_screen(fl::generated::ftxer::ScreenId declared) const override;
  ftxui::Component makeRaid() override;
  ftxui::Component makeParty() override;
  ftxui::Component makeAccountBattle() override;
  ftxui::Component makePartyBattle() override;
  ftxui::Component makeEffectGallery() override;
  ftxui::Component makeLibram() override;
  ftxui::Component makeBestiary() override;
  ftxui::Component makeObjectives() override;
  ftxui::Component makeVillage() override;
  ftxui::Component makeConsoleOverlay() override;
  ftxui::Component makeQuitConfirmationOverlay() override;
  ftxui::Component makeAutoplayConfirmationOverlay() override;
  ftxui::Component makeSandboxConfirmationOverlay() override;
  ftxui::Component makeKeybindHelpOverlay() override;
  ftxui::Element decorate(fl::generated::ftxer::ScreenId screen, ftxui::Element content) override;
  bool intercept(const ftxui::Event& event) override;
  void overlay_opened(fl::generated::ftxer::OverlayId overlay) override;
  void overlay_closed(fl::generated::ftxer::OverlayId overlay) override;
  bool collapse_help(fl::generated::ftxer::ScreenId current) override;
  bool expand_help(fl::generated::ftxer::ScreenId current) override;
  bool next_account(fl::generated::ftxer::ScreenId current) override;
  bool next_party(fl::generated::ftxer::ScreenId current) override;
  bool overdrive_down(fl::generated::ftxer::ScreenId current) override;
  bool overdrive_up(fl::generated::ftxer::ScreenId current) override;
  bool previous_account(fl::generated::ftxer::ScreenId current) override;
  bool previous_party(fl::generated::ftxer::ScreenId current) override;
  bool return_to_town(fl::generated::ftxer::ScreenId current) override;
  bool toggle_autoplay(fl::generated::ftxer::ScreenId current) override;
  bool toggle_help(fl::generated::ftxer::ScreenId current) override;
  bool quit_requested() const override;
  bool sandbox_requested() const override;

  RootComponent& root() {
    return *root_;
  }

private:
  std::unique_ptr<RootComponent> root_;
};

} // namespace fl::widgets
