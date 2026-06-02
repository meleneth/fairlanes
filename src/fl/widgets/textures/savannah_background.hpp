#pragma once

#include <cstdint>

#include <ftxui/dom/elements.hpp>

namespace fl::widgets::textures {

[[nodiscard]] std::uint32_t savannah_hash(std::uint32_t x, std::uint32_t y,
                                          std::uint32_t seed);

[[nodiscard]] ftxui::Element SavannahBackground(int width, int height,
                                                std::uint32_t seed);
[[nodiscard]] ftxui::Element SavannahPanel(ftxui::Element foreground, int width,
                                           int height, std::uint32_t seed);

} // namespace fl::widgets::textures
