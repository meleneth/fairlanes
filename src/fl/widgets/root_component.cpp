#include "root_component.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

#include <tracy/Tracy.hpp>

#include <ftxui/dom/elements.hpp>

#include "account_battle_view.hpp"
#include "console_overlay.hpp"
#include "fancy_log.hpp"
#include "moon_calendar_view.hpp"
#include "party_view.hpp"

namespace fl::widgets {

RootComponent::RootComponent(fl::context::AccountCtx ctx,
                             std::deque<fl::primitives::AccountData> &accounts,
                             FancyLog &console_log,
                             fl::primitives::WorldClock &world_clock)
    : ctx_(std::move(ctx)), accounts_(&accounts), console_log_(&console_log),
      world_clock_(&world_clock),
      commands_(accounts, console_log, world_clock) {
  console_overlay_ = ftxui::Make<ConsoleOverlay>(console_log_);
  commands_.set_show_account_view([this](std::size_t account_index) {
    show_account_battle(account_index);
  });
  commands_.set_show_party_view(
      [this](std::size_t account_index, std::size_t party_index) {
        show_party(account_index, party_index);
      });
  console_overlay()->set_on_command(
      [this](std::string_view command) { commands_.handle(command); });
  Add(console_overlay_);

  show_party(commands_.account_index(), commands_.party_index());
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
  content = vbox({
      render_moon_calendar(*world_clock_),
      separator(),
      content | flex,
  });

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

void RootComponent::show_account_battle(std::size_t account_index) {
  ctx_ = make_context(account_index);
  active_screen_ = ftxui::Make<AccountBattleView>(ctx_);
  Add(active_screen_);
}

void RootComponent::show_party(std::size_t account_index,
                               std::size_t party_index) {
  ctx_ = make_context(account_index);
  auto &parties = ctx_.account_data().parties();
  if (!parties.empty()) {
    party_index = std::min(party_index, parties.size() - 1);
  } else {
    party_index = 0;
  }

  active_screen_ = ftxui::Make<PartyView>(ctx_, party_index);
  Add(active_screen_);
}

fl::context::AccountCtx RootComponent::make_context(std::size_t account_index) {
  return fl::context::AccountCtx{ctx_.reg(), ctx_.rng(),
                                 accounts_->at(account_index)};
}

ConsoleOverlay *RootComponent::console_overlay() {
  return dynamic_cast<ConsoleOverlay *>(console_overlay_.get());
}

} // namespace fl::widgets
