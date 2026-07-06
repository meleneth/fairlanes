#pragma once

#include <cstdint>

#include <ftxui/component/component.hpp>
#include <ftxui/screen/color.hpp>

#include "fl/primitives/world_clock.hpp"

namespace fl::widgets {

class ChaosAttractRoot : public ftxui::ComponentBase {
public:
  ChaosAttractRoot(
      const fl::primitives::WorldClock *world_clock = nullptr,
      ftxui::Component battle_surface = nullptr);

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

  [[nodiscard]] bool begin_requested() const noexcept {
    return begin_requested_;
  }

  [[nodiscard]] ftxui::Color prompt_color(std::uint64_t frame) const;

private:
  ftxui::Element render_begin_prompt() const;

  const fl::primitives::WorldClock *world_clock_{nullptr};
  bool begin_requested_{false};
  std::uint64_t render_frames_{0};
  ftxui::Component battle_surface_;
};

} // namespace fl::widgets
