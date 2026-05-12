#include "root_component.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <tracy/Tracy.hpp>

#include <ftxui/dom/elements.hpp>

#include "account_battle_view.hpp"
#include "console_overlay.hpp"
#include "fancy_log.hpp"
#include "fl/lospec500.hpp"
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

  if (keybind_help_open_) {
    if (event == ftxui::Event::Character("h") ||
        event == ftxui::Event::Escape) {
      keybind_help_open_ = false;
      return true;
    }

    return true;
  }

  if (event == ftxui::Event::Character("`")) {
    toggle_console();
    return true;
  }

  if (overlay->open()) {
    return console_overlay_->OnEvent(event);
  }

  if (event == ftxui::Event::Character("h")) {
    keybind_help_open_ = true;
    return true;
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
      separator() | fl::lospec500::on_not_black(fl::lospec500::color_at(32)),
      content | flex,
      render_help_hint(),
  });

  if (keybind_help_open_) {
    content = dbox({
        content,
        render_keybind_help(),
    });
  }

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
  active_screen_kind_ = ActiveScreen::account_battle;
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

  active_screen_kind_ = ActiveScreen::party;
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

ftxui::Element RootComponent::render_help_hint() const {
  using namespace ftxui;

  const auto chrome = fl::lospec500::on_not_black(fl::lospec500::color_at(32));
  return hbox({
             filler(),
             text("h for help") | chrome,
         }) |
         bgcolor(fl::lospec500::color_at(0));
}

ftxui::Element RootComponent::render_keybind_help() const {
  using namespace ftxui;

  const auto chrome = fl::lospec500::on_not_black(fl::lospec500::color_at(32));
  const auto accent = fl::lospec500::on_not_black(fl::lospec500::color_at(15));
  const auto bg = bgcolor(fl::lospec500::color_at(0));

  std::vector<Element> lines;
  lines.push_back(text("Keybinds") | bold | accent);
  lines.push_back(separator() | chrome);
  lines.push_back(text("h        close this help") | chrome);
  lines.push_back(text("`        open command console") | chrome);
  lines.push_back(text("q / Esc  quit") | chrome);

  if (active_screen_kind_ == ActiveScreen::party) {
    lines.push_back(separator() | chrome);
    lines.push_back(text("Party") | bold | accent);
    lines.push_back(text("Tab      switch inventory / player details focus") |
                    chrome);
    lines.push_back(text("j / Down move inventory selection down") | chrome);
    lines.push_back(text("k / Up   move inventory selection up") | chrome);
    lines.push_back(text("PgUp/PgDn move inventory by pages") | chrome);
    lines.push_back(text("[ / ]    change selected player details") | chrome);
  } else {
    lines.push_back(separator() | chrome);
    lines.push_back(text("Account") | bold | accent);
    lines.push_back(text("Use the console for account and party navigation") |
                    chrome);
  }

  auto panel = window(text("Help") | accent, vbox(std::move(lines)) | bg) |
               chrome | bg | clear_under | size(WIDTH, GREATER_THAN, 44);

  return vbox({
      filler(),
      hbox({
          filler(),
          panel,
          filler(),
      }),
      filler(),
  });
}

} // namespace fl::widgets
