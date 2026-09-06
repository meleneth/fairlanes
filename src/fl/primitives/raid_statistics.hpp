#pragma once
#include "fl/events/raid_bus.hpp"

namespace fl::primitives {
struct RaidRecords {
  std::uint64_t attempts{0};
  std::uint64_t wins{0};
  std::uint64_t defeats{0};
  std::uint64_t mutual_destructions{0}; // subset of defeats
  std::uint64_t party_participations{0};
  std::uint64_t party_victory_credits{0};
  std::uint64_t trinkets_awarded{0};
  std::uint64_t combat_beats{0}; // completed raids only
};

// Account-owned through unique_ptr: subscriptions capture a stable address.
class RaidStatistics {
public:
  RaidStatistics(fl::events::RaidBus &bus, entt::entity account, bool enabled = true);
  const RaidRecords &records() const { return records_; }
private:
  void started(const fl::events::RaidStarted &event);
  void resolved(const fl::events::RaidResolved &event);
  fl::events::RaidBus &bus_;
  entt::entity account_;
  RaidRecords records_;
  std::uint64_t last_started_{0}, last_resolved_{0}, last_awarded_{0};
  fl::events::ScopedRaidListener start_sub_, resolved_sub_, loot_sub_;
};
} // namespace fl::primitives
