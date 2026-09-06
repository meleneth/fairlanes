#pragma once

#include "fl/events/party_bus.hpp"
#include <cstdint>

namespace fl::events {
enum class RaidResult { Victory, Defeat, MutualDestruction };
struct RaidStarted {
  std::uint64_t id;
  entt::entity account;
  std::size_t parties;
};
struct RaidResolved {
  std::uint64_t id;
  entt::entity account;
  RaidResult result;
  std::size_t parties;
  std::uint64_t combat_beats;
};
struct RaidRecorded { RaidResolved outcome; };
struct RaidLootAwarded { std::uint64_t id; entt::entity account; std::size_t items; };
using RaidEvent = std::variant<RaidStarted, RaidResolved, RaidRecorded, RaidLootAwarded>;
using RaidBus = seerin::VariantBus<RaidEvent>;
using ScopedRaidListener = ScopedListener<RaidBus>;
} // namespace fl::events
