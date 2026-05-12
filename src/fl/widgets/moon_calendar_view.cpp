#include "fl/widgets/moon_calendar_view.hpp"

#include <cstdint>
#include <string>

#include <ftxui/dom/elements.hpp>

#include "fl/lospec500.hpp"
#include "fl/primitives/moon_calendar.hpp"

namespace fl::widgets {
namespace {

std::string days_text(std::uint64_t days) {
  if (days == 0) {
    return "today";
  }
  return std::to_string(days) + "d";
}

ftxui::Element moon_status_line(const fl::primitives::MoonStatus &moon) {
  using namespace ftxui;

  return hbox({
      text(std::string(moon.name) + ": ") | bold,
      text(std::string(fl::primitives::MoonCalendar::phase_name(moon.phase))),
      text(" d" + std::to_string(moon.day_in_cycle) + "/" +
           std::to_string(moon.cycle_days)),
  });
}

} // namespace

ftxui::Element
render_moon_calendar(const fl::primitives::WorldClock &world_clock) {
  using namespace ftxui;
  const auto snapshot = fl::primitives::MoonCalendar::snapshot(world_clock);
  const auto chrome = fl::lospec500::color_at(32);
  const auto accent = fl::lospec500::color_at(15);

  auto primary = hbox({
      text("Moons") | bold | color(accent),
      text(" | Day " + std::to_string(snapshot.day)),
      text(" | "),
      moon_status_line(snapshot.runner),
      text(" | "),
      moon_status_line(snapshot.elder),
      filler(),
      text("x" + std::to_string(world_clock.beat_rate_multiplier()) + " "),
      text("Visitor " + days_text(snapshot.days_until_visitor)),
  });

  auto events = hbox({
      text("Runner Black " + days_text(snapshot.days_until_runner_black)),
      text(" | Elder Full " + days_text(snapshot.days_until_elder_full)),
      text(" | Split-Light " + days_text(snapshot.days_until_split_light)),
      text(" | Twin Dark " + days_text(snapshot.days_until_twin_dark)),
  });

  return vbox({
             primary,
             events | dim,
         }) |
         bgcolor(fl::lospec500::color_at(0)) | color(chrome);
}

} // namespace fl::widgets
