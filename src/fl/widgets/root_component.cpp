#include "root_component.hpp"

#include <utility>

#include <tracy/Tracy.hpp>

#include <ftxui/dom/elements.hpp>

#include "account_battle_view.hpp"
#include "console_overlay.hpp"
#include "fancy_log.hpp"

namespace fl::widgets {

RootComponent::RootComponent(fl::context::AccountCtx ctx)
    : ctx_(std::move(ctx)) {
  console_overlay_ = ftxui::Make<ConsoleOverlay>(&ctx_.log());
  Add(console_overlay_);

  show_account_battle();
}

bool RootComponent::OnEvent(ftxui::Event event) {
  auto *overlay = console_overlay();

  if (event == ftxui::Event::Character("`")) {
    toggle_console();
    return true;
  }

  if (overlay->open()) {
    return console_overlay_->OnEvent(event);
  }

  if (active_screen_) {
    return active_screen_->OnEvent(event);
  }

  return false;
}

ftxui::Element RootComponent::Render() {
  using namespace ftxui;

  ZoneScoped;

  auto *overlay = console_overlay();
  overlay->tick();

  Element content = active_screen_ ? active_screen_->Render() : text("");

  if (overlay->should_show()) {
    return dbox({
        content,
        console_overlay_->Render(),
    });
  }

  return content;
}

void RootComponent::toggle_console() {
  auto *overlay = console_overlay();
  overlay->toggle();

  if (overlay->open()) {
    overlay->FocusInput();
  }
}

void RootComponent::set_full_open() { console_overlay()->set_full_open(); }

void RootComponent::show_account_battle() {
  // For now, party 0 is the default battle screen.
  active_screen_ = ftxui::Make<AccountBattleView>(ctx_);
  Add(active_screen_);
}

ConsoleOverlay *RootComponent::console_overlay() {
  return dynamic_cast<ConsoleOverlay *>(console_overlay_.get());
}

} // namespace fl::widgets
