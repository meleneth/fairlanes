#include "fl/widgets/textures/forest_background.hpp"

#include "fl/lospec500.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace fl::widgets::textures {

std::uint32_t forest_hash(std::uint32_t x, std::uint32_t y,
                          std::uint32_t seed) {
  std::uint32_t h = seed ^ (x * 0x9E3779B9u) ^ (y * 0x85EBCA6Bu);
  h ^= h >> 16;
  h *= 0x7FEB352Du;
  h ^= h >> 15;
  h *= 0x846CA68Bu;
  h ^= h >> 16;
  return h;
}

ftxui::Element ForestBackground(int width, int height, std::uint32_t seed) {
  using namespace ftxui;

  width = std::max(1, width);
  height = std::max(1, height);

  std::vector<Element> rows;
  rows.reserve(static_cast<std::size_t>(height));

  for (int y = 0; y < height; ++y) {
    std::vector<Element> cells;
    cells.reserve(static_cast<std::size_t>(width));

    for (int x = 0; x < width; ++x) {
      const auto h = forest_hash(static_cast<std::uint32_t>(x),
                                 static_cast<std::uint32_t>(y), seed);
      const int r = static_cast<int>(h % 100);

      std::string glyph = " ";
      Color fg = fl::lospec500::color_at(16);
      Color bg = fl::lospec500::color_at(0);

      if (r < 4) {
        glyph = "^";
        fg = fl::lospec500::color_at(19);
      } else if (r < 7) {
        glyph = "♣";
        fg = fl::lospec500::color_at(18);
      } else if (r < 12) {
        glyph = "*";
        fg = fl::lospec500::color_at(17);
      } else if (r < 20) {
        glyph = ".";
        fg = fl::lospec500::color_at(16);
      } else if (r < 23) {
        glyph = "\"";
        fg = fl::lospec500::color_at(10);
      }

      cells.push_back(text(glyph) | color(fg) | bgcolor(bg));
    }

    rows.push_back(hbox(std::move(cells)));
  }

  return vbox(std::move(rows)) | size(WIDTH, EQUAL, width) |
         size(HEIGHT, EQUAL, height);
}

ftxui::Element ForestPanel(ftxui::Element foreground, int width, int height,
                           std::uint32_t seed) {
  using namespace ftxui;

  return dbox({
             ForestBackground(width, height, seed),
             std::move(foreground),
         }) |
         size(WIDTH, EQUAL, std::max(1, width)) |
         size(HEIGHT, EQUAL, std::max(1, height));
}

} // namespace fl::widgets::textures
