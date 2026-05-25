#pragma once

#include <entt/entt.hpp>

#include "fl/events/party_bus.hpp"

namespace fl::ecs::components {

struct StatusEffectInstance {
  entt::entity owner{entt::null};
  entt::entity effect_id{entt::null};
  fl::events::ScopedCombatantListener owner_died_sub;
  fl::events::ScopedPartyListener party_left_combat_sub;
  fl::events::ScopedPartyListener party_wiped_sub;
};

} // namespace fl::ecs::components
