#include "lospec500.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>

namespace fl::lospec500 {

std::span<const ftxui::Color> raw_colors() {
  static const std::array palette{
      ftxui::Color(16, 18, 28),    ftxui::Color(44, 30, 49),
      ftxui::Color(107, 38, 67),   ftxui::Color(172, 40, 71),
      ftxui::Color(236, 39, 63),   ftxui::Color(148, 73, 58),
      ftxui::Color(222, 93, 58),   ftxui::Color(233, 133, 55),
      ftxui::Color(243, 168, 51),  ftxui::Color(77, 53, 51),
      ftxui::Color(110, 76, 48),   ftxui::Color(162, 109, 63),
      ftxui::Color(206, 146, 72),  ftxui::Color(218, 177, 99),
      ftxui::Color(232, 210, 130), ftxui::Color(247, 243, 183),
      ftxui::Color(30, 64, 68),    ftxui::Color(0, 101, 84),
      ftxui::Color(38, 133, 76),   ftxui::Color(90, 181, 82),
      ftxui::Color(157, 230, 78),  ftxui::Color(0, 139, 139),
      ftxui::Color(98, 164, 119),  ftxui::Color(166, 203, 150),
      ftxui::Color(211, 238, 211), ftxui::Color(62, 59, 101),
      ftxui::Color(56, 89, 179),   ftxui::Color(51, 136, 222),
      ftxui::Color(54, 197, 244),  ftxui::Color(109, 234, 214),
      ftxui::Color(94, 91, 140),   ftxui::Color(140, 120, 165),
      ftxui::Color(176, 167, 184), ftxui::Color(222, 206, 237),
      ftxui::Color(154, 77, 118),  ftxui::Color(200, 120, 175),
      ftxui::Color(204, 153, 255), ftxui::Color(250, 110, 121),
      ftxui::Color(255, 162, 172), ftxui::Color(255, 209, 213),
      ftxui::Color(246, 232, 224), ftxui::Color(255, 255, 255),
  };

  return palette;
}

ftxui::Color color_at(std::size_t i) {
  auto palette = raw_colors();

  if (i >= palette.size()) {
    throw std::out_of_range("lospec500 palette index");
  }

  return palette[i];
}

ftxui::Decorator on_not_black(ftxui::Color color) {
  return ftxui::color(color) | ftxui::bgcolor(color_at(0));
}

std::span<const ftxui::Decorator> colors() {
  static const auto palette = [] {
    std::array<ftxui::Decorator, 42> out{};

    auto raw = raw_colors();
    for (std::size_t i = 0; i < raw.size(); ++i) {
      out[i] = fl::lospec500::on_not_black(raw[i]);
    }

    return out;
  }();

  return palette;
}

ftxui::Decorator at(std::size_t i) {
  auto palette = colors();

  if (i >= palette.size()) {
    throw std::out_of_range("lospec500 palette index");
  }

  return palette[i];
}

} // namespace fl::lospec500
