#pragma once
#include <cstdint>
#include <entt/entity/entity.hpp>

namespace fl::ecs::components {
// Raid-exclusive collectible; equipment slots and combat bonuses are undecided.
struct RaidTrinket {
  std::uint64_t raid_id;
  entt::entity account;
};
} // namespace fl::ecs::components
