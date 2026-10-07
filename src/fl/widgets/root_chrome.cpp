#include "root_chrome.hpp"

#include <string>
#include <utility>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "fl/lospec500.hpp"
#include "fl/widgets/moon_calendar_view.hpp"

namespace fl::widgets {

ftxui::Element render_help_keys(std::string_view keys) {
  std::vector<ftxui::Element> characters;
  for (char character : keys) {
    const bool separator = character == '/' && keys.size() > 1;
    auto element = ftxui::text(std::string(1, character)) |
                   ftxui::color(fl::lospec500::color_at(separator ? 25 : 15));
    if (!separator) {
      element = element | ftxui::bold;
    }
    characters.push_back(std::move(element));
  }
  return ftxui::hbox(std::move(characters));
}

ftxui::Element render_help_line(std::string_view keys,
                                std::string_view description) {
  std::vector<ftxui::Element> parts;
  parts.push_back(render_help_keys(keys));
  while (!description.empty()) {
    const auto slash = description.find('/');
    parts.push_back(ftxui::text(std::string(description.substr(0, slash))) |
                    ftxui::color(fl::lospec500::color_at(32)));
    if (slash == std::string_view::npos) {
      break;
    }
    parts.push_back(ftxui::text("/") |
                    ftxui::color(fl::lospec500::color_at(25)));
    description.remove_prefix(slash + 1);
  }
  return ftxui::hbox(std::move(parts));
}

ftxui::Element render_root_chrome(const fl::primitives::WorldClock &world_clock,
                                  ftxui::Element content,
                                  std::optional<VisitorCountdown> visitor) {
  return ftxui::vbox({
      render_moon_calendar(world_clock, visitor),
      ftxui::separator() |
          fl::lospec500::on_not_black(fl::lospec500::color_at(32)),
      std::move(content) | ftxui::flex,
  });
}

} // namespace fl::widgets
