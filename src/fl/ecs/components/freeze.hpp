#pragma once

#include <entt/entt.hpp>

#include "fl/events/party_bus.hpp"

namespace fl::ecs::components {

struct Freeze {
  entt::entity source{entt::null};
  int clear_after_beats{0};
  fl::events::ScopedPartyListener player_died_sub;
  fl::events::ScopedPartyListener left_combat_sub;
};

} // namespace fl::ecs::components
