#include "fill_bar.hpp"

#include <algorithm>
#include <array>
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

  static constexpr std::array<const char *, 8> kWidePartial = {
      "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█",
  };
  static constexpr std::array<const char *, 7> kNarrowPartial = {
      "╸", "╼", "━", "╾", "═", "╿", "█",
  };

  const float fill = clamp_fill(value_01);
  // Each cell is represented by a base wide-block state plus a narrow-box
  // refinement state to provide finer visual increments.
  static constexpr int kWideStepsPerCell = 8;
  static constexpr int kNarrowStepsPerWideStep =
      static_cast<int>(kNarrowPartial.size()) + 1;
  static constexpr int kTotalStepsPerCell =
      kWideStepsPerCell * kNarrowStepsPerWideStep;

  const int total_subcells = width * kTotalStepsPerCell;
  const int filled_subcells = std::clamp(
      static_cast<int>(std::lround(fill * static_cast<float>(total_subcells))),
      0, total_subcells);

  const int full_cells = filled_subcells / kTotalStepsPerCell;
  const int partial_subcells = filled_subcells % kTotalStepsPerCell;

  const int wide_level = partial_subcells / kNarrowStepsPerWideStep;
  const int narrow_level = partial_subcells % kNarrowStepsPerWideStep;

  std::string bar;
  bar.reserve(static_cast<std::size_t>(width) * 3U);

  for (int i = 0; i < full_cells; ++i) {
    bar += kWidePartial[7];
  }

  if (partial_subcells > 0 && full_cells < width) {
    if (wide_level == 0 && narrow_level > 0) {
      bar += kNarrowPartial[static_cast<std::size_t>(narrow_level - 1)];
    } else if (wide_level > 0 && narrow_level == 0) {
      bar += kWidePartial[static_cast<std::size_t>(wide_level - 1)];
    } else if (wide_level > 0) {
      // Keep visible progress beyond the wide step by choosing the next wider
      // block when a narrow refinement step is present.
      const int widened_level = std::min(wide_level, kWideStepsPerCell - 1);
      bar += kWidePartial[static_cast<std::size_t>(widened_level)];
    }
  }

  const int used_cells = full_cells + (partial_subcells > 0 ? 1 : 0);
  for (int i = used_cells; i < width; ++i) {
    bar += ' ';
  }

  return bar;
}

ftxui::Element labeled_fill_bar(std::string_view label, float value_01,
                                int width, std::string_view suffix) {
  const auto line = std::string(label) + ": [" + make_fill_bar(value_01, width) +
                    "] " + std::string(suffix);
  return ftxui::hbox({
      ftxui::text(line),
      ftxui::filler(),
  });
}

} // namespace fl::widgets
