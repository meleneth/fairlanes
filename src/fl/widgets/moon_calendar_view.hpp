#pragma once

#include <ftxui/dom/elements.hpp>

#include "fl/primitives/world_clock.hpp"

namespace fl::widgets {

ftxui::Element
render_moon_calendar(const fl::primitives::WorldClock &world_clock);

} // namespace fl::widgets
