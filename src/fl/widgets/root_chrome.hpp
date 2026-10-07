#pragma once

#include <string_view>

#include <ftxui/dom/elements.hpp>

#include "fl/primitives/world_clock.hpp"
#include "fl/widgets/moon_calendar_view.hpp"

namespace fl::widgets {

ftxui::Element render_help_keys(std::string_view keys);
ftxui::Element render_help_line(std::string_view keys,
                                std::string_view description);

ftxui::Element
render_root_chrome(const fl::primitives::WorldClock &world_clock,
                   ftxui::Element content,
                   std::optional<VisitorCountdown> visitor = std::nullopt);

} // namespace fl::widgets
