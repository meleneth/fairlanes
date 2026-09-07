#pragma once

#include <entt/entity/registry.hpp>

#include "fl/context.hpp"
#include "fl/events/party_bus.hpp"

namespace fl::ecs::systems {

class LootDropSystem {
public:
  static fl::events::ScopedPartyListener
  bind_listener(fl::context::PartyCtx &party_ctx);

  static void commit(fl::context::PartyCtx &party_ctx, entt::entity source);
};

} // namespace fl::ecs::systems
