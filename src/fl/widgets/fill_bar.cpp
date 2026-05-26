#include "fill_bar.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace fl::widgets {

float clamp_fill(float value_01) noexcept {
  return std::clamp(value_01, 0.0f, 1.0f);
}

std::string make_fill_bar(float value_01, int width) {
  if (width <= 0) {
    return {};
  }

  static constexpr const char *kHalfCell = "▌";
  static constexpr const char *kFullCell = "█";

  const float fill = clamp_fill(value_01);
  const int total_half_cells = width * 2;
  const int filled_half_cells =
      std::clamp(static_cast<int>(
                     std::lround(fill * static_cast<float>(total_half_cells))),
                 0, total_half_cells);

  const int full_cells = filled_half_cells / 2;
  const bool has_half_cell = (filled_half_cells % 2) != 0;

  std::string bar;
  bar.reserve(static_cast<std::size_t>(width) * 3U);

  for (int i = 0; i < full_cells; ++i) {
    bar += kFullCell;
  }

  if (has_half_cell && full_cells < width) {
    bar += kHalfCell;
  }

  const int used_cells = full_cells + (has_half_cell ? 1 : 0);
  for (int i = used_cells; i < width; ++i) {
    bar += ' ';
  }

  return bar;
}

ftxui::Element labeled_fill_bar(std::string_view label, float value_01,
                                int width, std::string_view suffix) {
  const auto line = std::string(label) + ": [" +
                    make_fill_bar(value_01, width) + "] " + std::string(suffix);
  return ftxui::hbox({
      ftxui::text(line),
      ftxui::filler(),
  });
}

} // namespace fl::widgets
