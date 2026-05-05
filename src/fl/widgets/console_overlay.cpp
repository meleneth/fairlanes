#include "console_overlay.hpp"

#include <algorithm>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/lospec500.hpp"
#include "fancy_log.hpp"

namespace fl::widgets {

ConsoleOverlay::ConsoleOverlay(FancyLog *console) : console_(console) {
  input_ = ftxui::Input(&line_);
  Add(input_);
}

bool ConsoleOverlay::open() const { return open_; }

bool ConsoleOverlay::should_show() const { return open_; }

void ConsoleOverlay::toggle() { set_open(!open_); }

void ConsoleOverlay::set_open(bool v) {
  open_ = v;

  if (!open_) {
    target_rows_ = 0;
    return;
  }

  target_rows_ = should_open_full_ ? ftxui::HEIGHT : 22;
}

void ConsoleOverlay::set_full_open() {
  should_open_full_ = !should_open_full_;
  set_open(true);
}

void ConsoleOverlay::tick() {
  if (rows_ == target_rows_) {
    return;
  }

  rows_ += rows_ < target_rows_ ? 1 : -1;
}

void ConsoleOverlay::change_console(FancyLog *console) {
  console_ = console;
}

void ConsoleOverlay::set_on_command(CommandHandler handler) {
  on_command_ = handler ? std::move(handler) : CommandHandler{[](std::string_view) {}};
}

void ConsoleOverlay::FocusInput() {
  if (input_) {
    input_->TakeFocus();
  }
}

bool ConsoleOverlay::OnEvent(ftxui::Event event) {
  if (!open_) {
    return false;
  }

  if (event == ftxui::Event::Return) {
    if (!line_.empty()) {
      if (console_) {
        console_->append_plain("> " + line_);
      }

      on_command_(line_);
      line_.clear();
    }

    FocusInput();
    return true;
  }

  if (input_ && input_->OnEvent(event)) {
    return true;
  }

  if (event == ftxui::Event::Escape) {
    set_open(false);
    return true;
  }

  return true;
}
ftxui::Element ConsoleOverlay::Render() {
  using namespace ftxui;

  auto chrome = fl::lospec500::on_not_black(fl::lospec500::color_at(32));
  auto bg = bgcolor(fl::lospec500::color_at(0));

  auto input_bar = hbox({
      text("> ") | chrome,
      input_ ? input_->Render() | chrome : text(""),
  }) | bg;

  Element log_el = console_ ? console_->Render() : text("");
  log_el = log_el | vscroll_indicator | yframe | yflex_grow | bg;

  auto content = vbox({
      log_el,
      separator() | chrome,
      input_bar,
  }) | bg;

  return window(text("Console") | chrome, content) |
         xflex_grow |
         clear_under |
         chrome |
         bg |
         size(HEIGHT, EQUAL, std::max(0, rows_));
}

} // namespace fl::widgets
