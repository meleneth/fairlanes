#pragma once

#include <cstdint>

#include <ftxui/dom/elements.hpp>

namespace fl::widgets::textures {

[[nodiscard]] std::uint32_t bog_hash(std::uint32_t x, std::uint32_t y,
                                     std::uint32_t seed);

[[nodiscard]] ftxui::Element
BogBackground(int width, int height, std::uint32_t seed, int bubble_frame);
[[nodiscard]] ftxui::Element BogPanel(ftxui::Element foreground, int width,
                                      int height, std::uint32_t seed);

} // namespace fl::widgets::textures
