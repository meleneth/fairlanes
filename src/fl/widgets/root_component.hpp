#pragma once

#include <ftxui/component/component.hpp>

#include "fl/context.hpp"
#include "fl/fwd.hpp"

namespace fl::widgets {

class ConsoleOverlay;
class FancyLog;

class RootComponent : public ftxui::ComponentBase {
public:
  explicit RootComponent(fl::context::AccountCtx ctx);

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

  void toggle_console();
  void set_full_open();

  void show_account_battle();
  //void show_party_battle(std::size_t party_index);

private:
  ConsoleOverlay *console_overlay();

  fl::context::AccountCtx ctx_;

  ftxui::Component active_screen_;
  ftxui::Component console_overlay_;
};

} // namespace fl::widgets
