#include "root_component.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fl/tracy_shim.hpp"
#include <ftxui/dom/elements.hpp>

#include "account_battle_view.hpp"
#include "console_overlay.hpp"
#include "effect_gallery_view.hpp"
#include "fl/lospec500.hpp"
#include "moon_calendar_view.hpp"
#include "party_battle_screen.hpp"
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
  commands_.set_show_effect_gallery([this]() { show_effect_gallery(); });
  console_overlay()->set_on_command(
      [this](std::string_view command) { commands_.handle(command); });
  Add(console_overlay_);

  show_party_battle(commands_.account_index(), commands_.party_index());
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

  if (active_screen_kind_ == ActiveScreen::effect_gallery &&
      (event == ftxui::Event::Escape ||
       event == ftxui::Event::Character("q"))) {
    show_party_battle(commands_.account_index(), commands_.party_index());
    return true;
  }

  if (active_screen_kind_ == ActiveScreen::effect_gallery && active_screen_ &&
      active_screen_->OnEvent(event)) {
    return true;
  }

  if (event == ftxui::Event::Character("h") &&
      active_screen_kind_ != ActiveScreen::effect_gallery) {
    keybind_help_open_ = true;
    return true;
  }

  if (event == ftxui::Event::Character(" ")) {
    toggle_party_battle_screen();
    return true;
  }

  if (event == ftxui::Event::Character("/")) {
    toggle_active_screen();
    return true;
  }

  if (event == ftxui::Event::Character("+")) {
    commands_.adjust_overdrive(1);
    return true;
  }

  if (event == ftxui::Event::Character("-")) {
    commands_.adjust_overdrive(-1);
    return true;
  }

  if (event == ftxui::Event::Character("[")) {
    const bool stay_on_party_battle =
        active_screen_kind_ == ActiveScreen::party_battle;
    commands_.select_party_relative(-1);
    if (stay_on_party_battle) {
      show_party_battle(commands_.account_index(), commands_.party_index());
    }
    return true;
  }

  if (event == ftxui::Event::Character("]")) {
    const bool stay_on_party_battle =
        active_screen_kind_ == ActiveScreen::party_battle;
    commands_.select_party_relative(1);
    if (stay_on_party_battle) {
      show_party_battle(commands_.account_index(), commands_.party_index());
    }
    return true;
  }

  if (event == ftxui::Event::Character("{")) {
    const bool stay_on_party_battle =
        active_screen_kind_ == ActiveScreen::party_battle;
    commands_.select_account_relative(-1);
    if (stay_on_party_battle) {
      show_party_battle(commands_.account_index(), commands_.party_index());
    }
    return true;
  }

  if (event == ftxui::Event::Character("}")) {
    const bool stay_on_party_battle =
        active_screen_kind_ == ActiveScreen::party_battle;
    commands_.select_account_relative(1);
    if (stay_on_party_battle) {
      show_party_battle(commands_.account_index(), commands_.party_index());
    }
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
  update_fps_counter();

  Element content = active_screen_ ? active_screen_->Render() : text("");
  content = vbox({
      render_moon_calendar(*world_clock_),
      separator() | fl::lospec500::on_not_black(fl::lospec500::color_at(32)),
      content | flex,
  });

  content = dbox({
      content,
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

void RootComponent::show_party_battle(std::size_t account_index,
                                      std::size_t party_index) {
  ctx_ = make_context(account_index);
  auto &parties = ctx_.account_data().parties();
  if (!parties.empty()) {
    party_index = std::min(party_index, parties.size() - 1);
  } else {
    party_index = 0;
  }

  active_screen_kind_ = ActiveScreen::party_battle;
  active_screen_ = ftxui::Make<PartyBattleScreen>(ctx_, party_index);
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

void RootComponent::show_effect_gallery() {
  active_screen_kind_ = ActiveScreen::effect_gallery;
  active_screen_ = ftxui::Make<EffectGalleryView>();
  Add(active_screen_);
}

void RootComponent::toggle_active_screen() {
  switch (active_screen_kind_) {
  case ActiveScreen::party:
    show_account_battle(commands_.account_index());
    return;
  case ActiveScreen::account_battle:
  case ActiveScreen::party_battle:
    show_party(commands_.account_index(), commands_.party_index());
    return;
  case ActiveScreen::effect_gallery:
    show_party_battle(commands_.account_index(), commands_.party_index());
    return;
  }
}

void RootComponent::toggle_party_battle_screen() {
  switch (active_screen_kind_) {
  case ActiveScreen::account_battle:
    show_party_battle(commands_.account_index(), commands_.party_index());
    return;
  case ActiveScreen::party_battle:
    show_account_battle(commands_.account_index());
    return;
  case ActiveScreen::party:
    return;
  case ActiveScreen::effect_gallery:
    show_party_battle(commands_.account_index(), commands_.party_index());
    return;
  }
}

fl::context::AccountCtx RootComponent::make_context(std::size_t account_index) {
  return fl::context::AccountCtx{ctx_.reg(), ctx_.rng(),
                                 accounts_->at(account_index)};
}

ConsoleOverlay *RootComponent::console_overlay() {
  return dynamic_cast<ConsoleOverlay *>(console_overlay_.get());
}

void RootComponent::update_fps_counter() {
  ++render_frames_;
  const auto now = std::chrono::steady_clock::now();
  if (!fps_initialized_) {
    fps_initialized_ = true;
    fps_window_start_ = now;
    fps_frame_count_ = 0;
    displayed_fps_ = 0;
    return;
  }

  ++fps_frame_count_;
  const auto elapsed = now - fps_window_start_;
  if (elapsed < std::chrono::milliseconds{500}) {
    return;
  }

  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  if (elapsed_ms > 0) {
    displayed_fps_ =
        static_cast<int>((static_cast<double>(fps_frame_count_) * 1000.0 /
                          static_cast<double>(elapsed_ms)) +
                         0.5);
  }

  fps_frame_count_ = 0;
  fps_window_start_ = now;
}

ftxui::Element RootComponent::render_help_hint() const {
  using namespace ftxui;

  const auto label = color(fl::lospec500::color_at(14));
  const auto value = color(fl::lospec500::color_at(28)) | bold;
  const auto key = color(fl::lospec500::color_at(22)) | bold;
  const auto edge = color(fl::lospec500::color_at(15));
  const auto muted = color(fl::lospec500::color_at(24));

  auto chunk = [](std::string name, std::string value_text,
                  Decorator label_style, Decorator value_style) {
    return hbox({
        text(std::move(name)) | label_style,
        text(std::move(value_text)) | value_style,
    });
  };

  const auto account_count = accounts_ ? accounts_->size() : 0U;
  const auto party_count =
      accounts_ && commands_.account_index() < accounts_->size()
          ? accounts_->at(commands_.account_index()).parties().size()
          : 0U;

  auto metrics = hbox({
      chunk("FPS ", std::to_string(displayed_fps_), label, value),
      text("  ") | muted,
      chunk("OD ", "x" + std::to_string(world_clock_->beat_rate_multiplier()),
            label, value),
      text("  ") | muted,
      chunk("AC ",
            std::to_string(commands_.account_index() + 1) + "/" +
                std::to_string(account_count),
            label, value),
      text("  ") | muted,
      chunk("PT ",
            std::to_string(commands_.party_index() + 1) + "/" +
                std::to_string(party_count),
            label, value),
      text("  ") | muted,
      chunk("WT ", std::to_string(world_clock_->elapsed_beats()), label, value),
  });

  auto controls = hbox({
      text("+/-") | key,
      text(" overdrive  ") | muted,
      text("[/]") | key,
      text(" party  ") | muted,
      text("{/}") | key,
      text(" account  ") | muted,
      text("/") | key,
      text(" screen  ") | muted,
      text("Space") | key,
      text(" party battle  ") | muted,
      text("h") | key,
      text(" help") | muted,
  });

  auto panel = vbox({metrics, controls}) | border | edge | clear_under;

  return vbox({
      filler(),
      hbox({
          filler(),
          panel,
      }),
      text(""),
  });
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

  if (active_screen_kind_ == ActiveScreen::effect_gallery) {
    lines.push_back(separator() | chrome);
    lines.push_back(text("Effect Gallery") | bold | accent);
    lines.push_back(text("Left/Right or h/l/[ / ] cycle effects") | chrome);
    lines.push_back(text("Up/Down or k/j cycle backgrounds") | chrome);
    lines.push_back(text("1-6      set rendered combatant count") | chrome);
    lines.push_back(text("q / Esc  return to normal battle screen") | chrome);
  } else if (active_screen_kind_ == ActiveScreen::party) {
    lines.push_back(separator() | chrome);
    lines.push_back(text("Party") | bold | accent);
    lines.push_back(text("/        toggle account / party screen") | chrome);
    lines.push_back(text("+ / -    change overdrive level") | chrome);
    lines.push_back(text("[ / ]    switch selected party") | chrome);
    lines.push_back(text("{ / }    switch selected account") | chrome);
    lines.push_back(text("Tab      switch inventory / player details focus") |
                    chrome);
    lines.push_back(text("j / Down move inventory selection down") | chrome);
    lines.push_back(text("k / Up   move inventory selection up") | chrome);
    lines.push_back(text("PgUp/PgDn move inventory by pages") | chrome);
  } else {
    lines.push_back(separator() | chrome);
    lines.push_back(text("Account") | bold | accent);
    lines.push_back(text("/        toggle account / party screen") | chrome);
    lines.push_back(text("Space    toggle account / party battle") | chrome);
    lines.push_back(text("+ / -    change overdrive level") | chrome);
    lines.push_back(text("[ / ]    switch selected party") | chrome);
    lines.push_back(text("{ / }    switch selected account") | chrome);
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
