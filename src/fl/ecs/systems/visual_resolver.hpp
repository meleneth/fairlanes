#pragma once

#include <optional>

#include <entt/entt.hpp>
#include <ftxui/screen/color.hpp>

#include "sr/uWu.hpp"

namespace fl::ecs::systems {

class VisualResolver {
public:
  static void resolve(entt::registry &reg, seerin::uWu now);
  static void resolve_entity(entt::registry &reg, entt::entity entity,
                             seerin::uWu now);

  static std::optional<ftxui::Color>
  resolve_body_color(entt::registry &reg, entt::entity entity, seerin::uWu now);

  static std::optional<ftxui::Color> resolve_hp_bar_color(entt::registry &reg,
                                                          entt::entity entity,
                                                          seerin::uWu now);
};

} // namespace fl::ecs::systems
