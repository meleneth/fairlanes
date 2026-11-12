#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace fl::generated::ftxer {

enum class ScreenId { Raid, Party, AccountBattle, PartyBattle, EffectGallery, Libram, Bestiary, Objectives, Village };
enum class OverlayId { Console, QuitConfirmation, AutoplayConfirmation, SandboxConfirmation, KeybindHelp, CharacterManagement };

struct ShellCallbacks {
  std::function<bool(ScreenId, bool)> navigate;
  std::function<void(OverlayId)> open_overlay;
  std::function<void(OverlayId)> close_overlay;
  std::function<void()> refresh;
};

class ExternalUiAdapter {
 public:
  virtual ~ExternalUiAdapter() = default;
  virtual void bind_shell(ShellCallbacks callbacks) = 0;
  virtual ScreenId initial_screen(ScreenId declared) const = 0;
  virtual ftxui::Component makeRaid() = 0;
  virtual ftxui::Component makeParty() = 0;
  virtual ftxui::Component makeAccountBattle() = 0;
  virtual ftxui::Component makePartyBattle() = 0;
  virtual ftxui::Component makeEffectGallery() = 0;
  virtual ftxui::Component makeLibram() = 0;
  virtual ftxui::Component makeBestiary() = 0;
  virtual ftxui::Component makeObjectives() = 0;
  virtual ftxui::Component makeVillage() = 0;
  virtual ftxui::Component makeConsoleOverlay() = 0;
  virtual ftxui::Component makeQuitConfirmationOverlay() = 0;
  virtual ftxui::Component makeAutoplayConfirmationOverlay() = 0;
  virtual ftxui::Component makeSandboxConfirmationOverlay() = 0;
  virtual ftxui::Component makeKeybindHelpOverlay() = 0;
  virtual ftxui::Component makeCharacterManagementOverlay() = 0;
  virtual ftxui::Element decorate(ScreenId screen, ftxui::Element content) = 0;
  virtual bool intercept(const ftxui::Event& event) = 0;
  virtual void overlay_opened(OverlayId overlay) = 0;
  virtual void overlay_closed(OverlayId overlay) = 0;
  virtual bool collapse_help(ScreenId current) = 0;
  virtual bool expand_help(ScreenId current) = 0;
  virtual bool next_account(ScreenId current) = 0;
  virtual bool next_party(ScreenId current) = 0;
  virtual bool open_character_management(ScreenId current) = 0;
  virtual bool overdrive_down(ScreenId current) = 0;
  virtual bool overdrive_up(ScreenId current) = 0;
  virtual bool previous_account(ScreenId current) = 0;
  virtual bool previous_party(ScreenId current) = 0;
  virtual bool return_to_town(ScreenId current) = 0;
  virtual bool toggle_autoplay(ScreenId current) = 0;
  virtual bool toggle_help(ScreenId current) = 0;
  virtual bool quit_requested() const = 0;
  virtual bool sandbox_requested() const = 0;
};

class CompositionRoot : public ftxui::ComponentBase {
 public:
  explicit CompositionRoot(std::unique_ptr<ExternalUiAdapter> adapter);
  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;
  ftxui::Component ActiveChild() override;
  ScreenId current_screen() const { return current_screen_; }
  std::size_t history_size() const { return history_.size(); }
  bool console_open() const;
  bool quit_requested() const { return adapter_->quit_requested(); }
  bool sandbox_requested() const { return adapter_->sandbox_requested(); }

 private:
  bool navigate(ScreenId destination, bool remember = true, bool refresh = false);
  bool back();
  bool run_binding(const ftxui::Event& event);
  void refresh();
  void open_overlay(OverlayId overlay);
  void close_overlay(OverlayId overlay);
  ftxui::Component make_screen(ScreenId screen);
  ftxui::Component overlay_component(OverlayId overlay) const;

  std::unique_ptr<ExternalUiAdapter> adapter_;
  ftxui::Component active_screen_;
  ftxui::Component console_overlay_;
  ftxui::Component quit_confirmation_overlay_;
  ftxui::Component autoplay_confirmation_overlay_;
  ftxui::Component sandbox_confirmation_overlay_;
  ftxui::Component keybind_help_overlay_;
  ftxui::Component character_management_overlay_;
  ScreenId current_screen_{ScreenId::PartyBattle};
  std::vector<ScreenId> history_;
  std::vector<OverlayId> overlays_;
};

}  // namespace fl::generated::ftxer
