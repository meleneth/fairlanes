#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <vector>

#include <ftxui/screen/color.hpp>

#include "fl/lospec500.hpp"
#include "fl/widgets/effects/decal.hpp"

namespace fl::widgets::effects::detail {

[[nodiscard]] inline float clamp01(float value) {
  return std::clamp(value, 0.0f, 1.0f);
}

[[nodiscard]] inline float clamp_progress(float progress) {
  if (std::isnan(progress)) {
    return 0.0f;
  }
  return clamp01(progress);
}

[[nodiscard]] inline ftxui::Color palette(std::size_t index) {
  return fl::lospec500::color_at(index);
}

[[nodiscard]] inline ftxui::Color lerp_color(ftxui::Color a, ftxui::Color b,
                                             float t) {
  return clamp01(t) < 0.5F ? a : b;
}

[[nodiscard]] inline bool in_bounds(int x, int y, int width, int height) {
  return x >= 0 && y >= 0 && x < width && y < height;
}

inline void apply_cell(Frame &frame, int x, int y, char glyph,
                       std::optional<ftxui::Color> fg,
                       std::optional<ftxui::Color> bg, float alpha) {
  if (!in_bounds(x, y, frame.width, frame.height)) {
    return;
  }

  auto &cell = frame.at(x, y);
  cell.alpha = std::max(cell.alpha, clamp01(alpha));
  if (glyph != ' ') {
    cell.glyph = glyph;
  }
  if (fg) {
    cell.fg = *fg;
  }
  if (bg) {
    cell.bg = *bg;
  }
}

inline void apply_bg_glow(Frame &frame, int cx, int cy, int radius,
                          ftxui::Color color, float alpha) {
  for (int y = cy - radius; y <= cy + radius; ++y) {
    for (int x = cx - radius; x <= cx + radius; ++x) {
      if (!in_bounds(x, y, frame.width, frame.height)) {
        continue;
      }
      const int dx = x - cx;
      const int dy = y - cy;
      const float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
      if (dist > static_cast<float>(radius)) {
        continue;
      }
      const float fade =
          1.0F - dist / std::max(1.0F, static_cast<float>(radius));
      apply_cell(frame, x, y, ' ', std::nullopt, color, alpha * fade);
    }
  }
}

[[nodiscard]] inline Frame make_frame(int width, int height) {
  if (width <= 0 || height <= 0) {
    return {};
  }
  return {width, height,
          std::vector<RenderCell>(static_cast<std::size_t>(width * height))};
}

} // namespace fl::widgets::effects::detail
