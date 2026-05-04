#pragma once

#include <cstddef>
#include <span>

#include <ftxui/dom/elements.hpp>

namespace fl::lospec500 {

std::span<const ftxui::Color> raw_colors();
ftxui::Color color_at(std::size_t i);

std::span<const ftxui::Decorator> colors();
ftxui::Decorator at(std::size_t i);

} // namespace fl::lospec500
