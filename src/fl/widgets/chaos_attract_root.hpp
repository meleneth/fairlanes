#pragma once

#include <cstdint>

#include <ftxui/component/component.hpp>
#include <ftxui/screen/color.hpp>

namespace fl::widgets {

class ChaosAttractRoot : public ftxui::ComponentBase {
public:
  explicit ChaosAttractRoot(ftxui::Component battle_surface = nullptr);

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

  [[nodiscard]] bool begin_requested() const noexcept {
    return begin_requested_;
  }
  [[nodiscard]] bool help_open() const noexcept { return help_open_; }

  [[nodiscard]] ftxui::Color prompt_color(std::uint64_t frame) const;

private:
  ftxui::Element render_begin_prompt() const;
  ftxui::Element render_help_panel() const;

  bool begin_requested_{false};
  bool help_open_{false};
  std::uint64_t render_frames_{0};
  ftxui::Component battle_surface_;
};

} // namespace fl::widgets
