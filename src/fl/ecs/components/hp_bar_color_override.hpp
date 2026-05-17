#pragma once

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>

#include "fl/ecs/components/visual_effects.hpp"

namespace fl::ecs::components {

using HPBarColorOverride = ResolvedHPBarColorOverride;

inline void safe_add_hp_bar_color(entt::registry &reg, entt::entity e,
                                  ftxui::Color c) {
  if (!reg.valid(e)) {
    return;
  }
  auto &status = reg.get_or_emplace<StatusTint>(e);
  status.hp_bar_color = c;
}

inline void safe_clear_hp_bar_color(entt::registry &reg, entt::entity e) {
  if (!reg.valid(e)) {
    return;
  }
  if (auto *status = reg.try_get<StatusTint>(e)) {
    status->hp_bar_color.reset();
    if (!status->body_color && !status->hp_bar_color &&
        !status->background_color) {
      reg.remove<StatusTint>(e);
    }
  }
}

} // namespace fl::ecs::components
