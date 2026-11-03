#include "story_start_picker.hpp"

#include <string>
#include <utility>

#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/lospec500.hpp"

namespace fl::widgets {

StoryStartPicker::StoryStartPicker(std::function<void()> finish)
    : finish_(std::move(finish)) {}

bool StoryStartPicker::OnEvent(ftxui::Event event) {
  if (confirming_autoplay_) {
    if (event == ftxui::Event::Return || event == ftxui::Event::Character("y")) {
      mode_ = StoryStartMode::Autoplay;
      finish_();
    } else if (event == ftxui::Event::Escape ||
               event == ftxui::Event::Character("n")) {
      confirming_autoplay_ = false;
    }
    return true;
  }
  if (event == ftxui::Event::ArrowDown || event == ftxui::Event::Character("j") ||
      event == ftxui::Event::ArrowUp || event == ftxui::Event::Character("k")) {
    selected_ = 1 - selected_;
    return true;
  }
  if (event == ftxui::Event::Return) {
    if (selected_ == 1) confirming_autoplay_ = true;
    else {
      mode_ = StoryStartMode::Manual;
      finish_();
    }
    return true;
  }
  if (event == ftxui::Event::Escape || event == ftxui::Event::Character("q")) {
    mode_ = StoryStartMode::Cancel;
    finish_();
    return true;
  }
  return false;
}

ftxui::Element StoryStartPicker::Render() {
  using namespace ftxui;
  if (confirming_autoplay_) {
    auto panel = window(
        text("Enable story autoplay?") | bold,
        vbox({paragraph("Autoplay will choose dialogue, return to town, buy gear, and recruit heroes for you."),
              text("You can pause it with A at any time."),
              separator(), text("Enter/y: enable   Esc/n: choose again")})) |
                 bgcolor(fl::lospec500::color_at(0)) |
                 color(fl::lospec500::color_at(32)) | clear_under;
    return vbox({filler(), hbox({filler(), panel, filler()}), filler()});
  }
  auto panel = window(
      text("Begin the story") | bold,
      vbox({paragraph("The first Visitor has reached the village."), separator(),
            text(std::string(selected_ == 0 ? "> " : "  ") +
                 "Manual story — choose dialogue and objectives yourself"),
            text(std::string(selected_ == 1 ? "> " : "  ") +
                 "Autoplay — follow the authored opening route"),
            separator(),
            text("Press A to toggle autoplay at any time during the story.") |
                bold,
            text("Up/Down choose   Enter begin   Esc cancel") | dim})) |
               bgcolor(fl::lospec500::color_at(0)) |
               color(fl::lospec500::color_at(32)) | clear_under;
  return vbox({filler(), hbox({filler(), panel, filler()}), filler()});
}

StoryStartMode choose_story_start_mode() {
  auto screen = ftxui::ScreenInteractive::Fullscreen();
  auto picker = ftxui::Make<StoryStartPicker>([&screen] { screen.Exit(); });
  const auto component = picker;
  screen.Loop(component);
  return picker->mode();
}

} // namespace fl::widgets
