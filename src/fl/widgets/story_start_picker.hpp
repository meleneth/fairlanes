#pragma once

#include <functional>

#include <ftxui/component/component.hpp>

namespace fl::widgets {

enum class StoryStartMode { Manual, Autoplay, Cancel };

class StoryStartPicker : public ftxui::ComponentBase {
public:
  explicit StoryStartPicker(std::function<void()> finish);
  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;
  StoryStartMode mode() const noexcept {
    return mode_;
  }

private:
  std::function<void()> finish_;
  int selected_{0};
  bool confirming_autoplay_{false};
  StoryStartMode mode_{StoryStartMode::Cancel};
};

StoryStartMode choose_story_start_mode();

} // namespace fl::widgets
