#pragma once

#include <ftxui/dom/elements.hpp>

#include "fl/primitives/world_clock.hpp"

namespace fl::widgets {

ftxui::Element render_root_chrome(
    const fl::primitives::WorldClock &world_clock, ftxui::Element content);

} // namespace fl::widgets
