#pragma once

#include "fl/context.hpp"
#include "fl/events/party_bus.hpp"

namespace fl::ecs::systems {

class ManualEquipmentSystem {
public:
  static fl::events::ScopedPartyListener bind_listener(fl::context::PartyCtx& party_ctx);

  static bool commit(fl::context::PartyCtx& party_ctx,
                     const fl::events::EquipmentEquipRequested& request);
};

} // namespace fl::ecs::systems
