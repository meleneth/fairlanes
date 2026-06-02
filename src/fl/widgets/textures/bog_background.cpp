#include "fl/widgets/textures/bog_background.hpp"

#include "fl/lospec500.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace fl::widgets::textures {
namespace {

[[nodiscard]] int current_bubble_frame() {
  using clock = std::chrono::steady_clock;
  using seconds = std::chrono::seconds;

  const auto elapsed =
      std::chrono::duration_cast<seconds>(clock::now().time_since_epoch());
  return static_cast<int>((elapsed.count() / 4) % 4);
}

} // namespace

std::uint32_t bog_hash(std::uint32_t x, std::uint32_t y, std::uint32_t seed) {
  std::uint32_t h = seed ^ (x * 0xA24BAED5u) ^ (y * 0x9FB21C65u);
  h ^= h >> 16;
  h *= 0x21F0AAADu;
  h ^= h >> 15;
  h *= 0x735A2D97u;
  h ^= h >> 16;
  return h;
}

ftxui::Element BogBackground(int width, int height, std::uint32_t seed,
                             int bubble_frame) {
  using namespace ftxui;

  width = std::max(1, width);
  height = std::max(1, height);
  bubble_frame = ((bubble_frame % 4) + 4) % 4;

  std::vector<Element> rows;
  rows.reserve(static_cast<std::size_t>(height));

  for (int y = 0; y < height; ++y) {
    std::vector<Element> cells;
    cells.reserve(static_cast<std::size_t>(width));

    for (int x = 0; x < width; ++x) {
      const auto h = bog_hash(static_cast<std::uint32_t>(x),
                              static_cast<std::uint32_t>(y), seed);
      const int r = static_cast<int>(h % 100);
      const int phase = static_cast<int>((h >> 8) & 3u);

      std::string glyph = " ";
      Color fg = fl::lospec500::color_at(25);
      Color bg = fl::lospec500::color_at(0);

      if (r < 3) {
        glyph = ",";
        fg = fl::lospec500::color_at(30);
      } else if (r < 7) {
        glyph = ".";
        fg = fl::lospec500::color_at(25);
      } else if (r < 10) {
        glyph = "~";
        fg = fl::lospec500::color_at(26);
      } else if (r < 13) {
        glyph = "*";
        fg = fl::lospec500::color_at(34);
      } else if (r < 17) {
        glyph = "'";
        fg = fl::lospec500::color_at(2);
      } else if (r < 21) {
        const int age = (bubble_frame + 4 - phase) % 4;
        if (age == 1) {
          glyph = ".";
          fg = fl::lospec500::color_at(27);
        } else if (age == 2) {
          glyph = "o";
          fg = fl::lospec500::color_at(31);
        } else if (age == 3) {
          glyph = "O";
          fg = fl::lospec500::color_at(33);
        }
      }

      cells.push_back(text(glyph) | color(fg) | bgcolor(bg));
    }

    rows.push_back(hbox(std::move(cells)));
  }

  return vbox(std::move(rows)) | size(WIDTH, EQUAL, width) |
         size(HEIGHT, EQUAL, height);
}

ftxui::Element BogPanel(ftxui::Element foreground, int width, int height,
                        std::uint32_t seed) {
  using namespace ftxui;

  return dbox({
             BogBackground(width, height, seed, current_bubble_frame()),
             std::move(foreground),
         }) |
         size(WIDTH, EQUAL, std::max(1, width)) |
         size(HEIGHT, EQUAL, std::max(1, height));
}

} // namespace fl::widgets::textures
