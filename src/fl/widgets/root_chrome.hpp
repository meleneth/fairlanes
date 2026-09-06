#pragma once

#include <ftxui/dom/elements.hpp>

#include "fl/primitives/world_clock.hpp"
#include "fl/widgets/moon_calendar_view.hpp"

namespace fl::widgets {

ftxui::Element
render_root_chrome(const fl::primitives::WorldClock &world_clock,
                   ftxui::Element content,
                   std::optional<VisitorCountdown> visitor = std::nullopt);

} // namespace fl::widgets
