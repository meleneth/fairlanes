#pragma once

#include <optional>

#include <ftxui/screen/color.hpp>

#include "sr/uWu.hpp"

namespace fl::ecs::components {

struct ResolvedColorOverride {
  ftxui::Color color;
};

struct ResolvedHPBarColorOverride {
  ftxui::Color color;
};

struct DamageFlash {
  ftxui::Color color;
  seerin::uWu expires_at{};
};

struct ActiveGlow {
  ftxui::Color color;
  seerin::uWu expires_at{};
};

struct StatusTint {
  std::optional<ftxui::Color> body_color;
  std::optional<ftxui::Color> hp_bar_color;
};

struct BaseVisual {
  std::optional<ftxui::Color> body_color;
};

struct DeadVisual {
  ftxui::Color color;
};

} // namespace fl::ecs::components
