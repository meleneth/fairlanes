#pragma once

#include <entt/entity/fwd.hpp>

#include "fl/events/party_bus.hpp"

namespace fl::ecs::systems {

class CastingSkillSystem {
public:
  static fl::events::ScopedCombatantListener bind_started_listener(entt::registry& reg,
                                                                   fl::events::CombatantBus& bus);

  static fl::events::ScopedCombatantListener bind_finished_listener(entt::registry& reg,
                                                                    fl::events::CombatantBus& bus);
};

} // namespace fl::ecs::systems
