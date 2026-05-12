#pragma once

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>

namespace fl::ecs::components {

struct HPBarColorOverride {
  ftxui::Color color;
};

inline void safe_add_hp_bar_color(entt::registry &reg, entt::entity e,
                                  ftxui::Color c) {
  if (!reg.valid(e)) {
    return;
  }
  reg.emplace_or_replace<HPBarColorOverride>(e, HPBarColorOverride{c});
}

inline void safe_clear_hp_bar_color(entt::registry &reg, entt::entity e) {
  if (!reg.valid(e)) {
    return;
  }
  reg.remove<HPBarColorOverride>(e);
}

} // namespace fl::ecs::components
