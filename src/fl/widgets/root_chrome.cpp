#include "root_chrome.hpp"

#include <utility>

#include <ftxui/dom/elements.hpp>

#include "fl/lospec500.hpp"
#include "fl/widgets/moon_calendar_view.hpp"

namespace fl::widgets {

ftxui::Element render_root_chrome(
    const fl::primitives::WorldClock &world_clock, ftxui::Element content) {
  return ftxui::vbox({
      render_moon_calendar(world_clock),
      ftxui::separator() |
          fl::lospec500::on_not_black(fl::lospec500::color_at(32)),
      std::move(content) | ftxui::flex,
  });
}

} // namespace fl::widgets
