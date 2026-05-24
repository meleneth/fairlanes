#pragma once

#include <string>
#include <string_view>

#include <ftxui/dom/elements.hpp>

namespace fl::widgets {

[[nodiscard]] float clamp_fill(float value_01) noexcept;

[[nodiscard]] std::string make_fill_bar(float value_01, int width);

[[nodiscard]] ftxui::Element labeled_fill_bar(std::string_view label,
                                              float value_01, int width,
                                              std::string_view suffix);

} // namespace fl::widgets
