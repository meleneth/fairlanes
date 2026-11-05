#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fancy_log.hpp"

namespace fl::widgets {

class ConsoleOverlay : public ftxui::ComponentBase {
public:
  using CommandHandler = std::function<void(std::string_view)>;

  explicit ConsoleOverlay(FancyLog* console);

  bool open() const;
  bool should_show() const;
  std::size_t history_size() const {
    return history_.size();
  }
  const std::string& input_line() const {
    return line_;
  }

  void toggle();
  void set_open(bool v);
  void set_full_open();

  void tick();
  void change_console(FancyLog* console);
  void set_on_command(CommandHandler handler);

  bool OnEvent(ftxui::Event event) override;
  void FocusInput();

  ftxui::Element Render() override;

private:
  bool open_ = false;
  bool should_open_full_ = false;

  int rows_ = 0;
  int target_rows_ = 0;

  FancyLog* console_ = nullptr;

  std::string line_;
  std::string history_draft_;
  std::vector<std::string> history_;
  std::size_t history_cursor_{0};
  ftxui::Component input_;

  CommandHandler on_command_ = [](std::string_view) {};
};

} // namespace fl::widgets
