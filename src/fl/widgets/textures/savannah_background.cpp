#include "fl/widgets/textures/savannah_background.hpp"

#include "fl/lospec500.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace fl::widgets::textures {

std::uint32_t savannah_hash(std::uint32_t x, std::uint32_t y,
                            std::uint32_t seed) {
  std::uint32_t h = seed ^ (x * 0xD1B54A35u) ^ (y * 0x94D049BBu);
  h ^= h >> 15;
  h *= 0x2C1B3C6Du;
  h ^= h >> 12;
  h *= 0x297A2D39u;
  h ^= h >> 15;
  return h;
}

ftxui::Element SavannahBackground(int width, int height, std::uint32_t seed) {
  using namespace ftxui;

  width = std::max(1, width);
  height = std::max(1, height);

  std::vector<Element> rows;
  rows.reserve(static_cast<std::size_t>(height));

  for (int y = 0; y < height; ++y) {
    std::vector<Element> cells;
    cells.reserve(static_cast<std::size_t>(width));

    for (int x = 0; x < width; ++x) {
      const auto h = savannah_hash(static_cast<std::uint32_t>(x),
                                   static_cast<std::uint32_t>(y), seed);
      const int r = static_cast<int>(h % 100);

      std::string glyph = " ";
      Color fg = fl::lospec500::color_at(11);
      Color bg = fl::lospec500::color_at(0);

      if (r < 2) {
        glyph = "^";
        fg = fl::lospec500::color_at(13);
      } else if (r < 5) {
        glyph = "'";
        fg = fl::lospec500::color_at(12);
      } else if (r < 8) {
        glyph = ".";
        fg = fl::lospec500::color_at(11);
      } else if (r < 10) {
        glyph = "*";
        fg = fl::lospec500::color_at(8);
      } else if (r < 13) {
        glyph = "o";
        fg = fl::lospec500::color_at(30);
      }

      cells.push_back(text(glyph) | color(fg) | bgcolor(bg));
    }

    rows.push_back(hbox(std::move(cells)));
  }

  return vbox(std::move(rows)) | size(WIDTH, EQUAL, width) |
         size(HEIGHT, EQUAL, height);
}

ftxui::Element SavannahPanel(ftxui::Element foreground, int width, int height,
                             std::uint32_t seed) {
  using namespace ftxui;

  return dbox({
             SavannahBackground(width, height, seed),
             std::move(foreground),
         }) |
         size(WIDTH, EQUAL, std::max(1, width)) |
         size(HEIGHT, EQUAL, std::max(1, height));
}

} // namespace fl::widgets::textures
