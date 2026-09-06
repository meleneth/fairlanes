#pragma once

#include <ftxui/dom/elements.hpp>
#include <optional>

#include "fl/primitives/world_clock.hpp"

namespace fl::widgets {

struct VisitorCountdown {
  std::uint64_t remaining_beats;
  bool paused;
};
ftxui::Element
render_moon_calendar(const fl::primitives::WorldClock &world_clock,
                     std::optional<VisitorCountdown> visitor = std::nullopt);

} // namespace fl::widgets
