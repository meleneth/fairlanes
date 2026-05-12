#pragma once

#include <cstddef>
#include <deque>

#include <ftxui/component/component.hpp>

#include "fl/context.hpp"
#include "fl/fwd.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/widgets/ui_command_controller.hpp"

namespace fl::widgets {

class ConsoleOverlay;
class FancyLog;

class RootComponent : public ftxui::ComponentBase {
public:
  RootComponent(fl::context::AccountCtx ctx,
                std::deque<fl::primitives::AccountData> &accounts,
                FancyLog &console_log,
                fl::primitives::WorldClock &world_clock);

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

  void toggle_console();
  void set_full_open();

  void show_account_battle(std::size_t account_index);
  void show_party(std::size_t account_index, std::size_t party_index);

private:
  enum class ActiveScreen {
    party,
    account_battle,
  };

  ConsoleOverlay *console_overlay();
  fl::context::AccountCtx make_context(std::size_t account_index);
  ftxui::Element render_help_hint() const;
  ftxui::Element render_keybind_help() const;

  fl::context::AccountCtx ctx_;
  std::deque<fl::primitives::AccountData> *accounts_{nullptr};
  FancyLog *console_log_{nullptr};
  fl::primitives::WorldClock *world_clock_{nullptr};
  UiCommandController commands_;
  ActiveScreen active_screen_kind_{ActiveScreen::party};
  bool keybind_help_open_{false};

  ftxui::Component active_screen_;
  ftxui::Component console_overlay_;
};

} // namespace fl::widgets
