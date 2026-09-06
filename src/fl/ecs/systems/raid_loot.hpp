#pragma once
#include "fl/context.hpp"
#include "fl/events/raid_bus.hpp"
#include <span>

namespace fl::ecs::systems {
class RaidLootSystem {
public:
  static constexpr int kDropsPerParty = 2;
  // Called once by collective victory resolution, before RaidResolved is
  // emitted.
  static void commit(fl::context::AccountCtx &ctx, fl::events::RaidBus &bus,
                     std::uint64_t raid_id,
                     std::span<fl::primitives::PartyData *const> participants);
};
} // namespace fl::ecs::systems
