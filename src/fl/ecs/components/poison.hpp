#pragma once

#include <entt/entt.hpp>

#include "fl/events/party_bus.hpp"

namespace fl::ecs::components {

struct Poison {
  entt::entity source{entt::null};
  int damage_per_tick{0};
  int ticks_remaining{0};
  fl::events::ScopedPartyListener player_died_sub;
  fl::events::ScopedPartyListener left_combat_sub;
};

} // namespace fl::ecs::components
