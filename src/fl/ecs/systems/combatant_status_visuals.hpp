#pragma once

#include <optional>
#include <string>
#include <vector>

#include <entt/entt.hpp>
#include <ftxui/screen/color.hpp>

#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/visual_effects.hpp"

namespace fl::ecs::systems {

enum class CombatantVisualLayer {
  Underlay,
  Body,
  HpBar,
  StatusText,
  Overlay,
};

enum class CombatantVisualRegion {
  WholeCombatant,
  Nameplate,
  HpBar,
  Body,
  Feet,
  Head,
  StatusRows,
};

struct CombatantStatusVisual {
  CombatantVisualLayer layer{CombatantVisualLayer::StatusText};
  CombatantVisualRegion region{CombatantVisualRegion::StatusRows};
  int priority{0};
  std::string label;
  std::optional<ftxui::Color> color;
  std::optional<fl::ecs::components::CombatStatusKind> status_kind;
  std::optional<fl::ecs::components::DecalEffect> decal;
  bool preserves_existing_targeted_behavior{false};
};

[[nodiscard]] std::vector<CombatantStatusVisual>
combatant_status_visuals(entt::registry &reg, entt::entity entity);

[[nodiscard]] std::vector<CombatantStatusVisual>
combatant_status_visuals_for(entt::registry &reg, entt::entity entity,
                             CombatantVisualLayer layer,
                             CombatantVisualRegion region);

} // namespace fl::ecs::systems
