#pragma once

#include "fl/context.hpp"
#include "fl/events/raid_bus.hpp"
#include "fl/primitives/encounter_data.hpp"
#include <optional>
#include <chrono>
#include <span>
#include <vector>

namespace fl::primitives {

// Account-owned, nonmoving encounter. Parties borrow encounter_ until
// resolution. combat_bus_ and context dependencies outlive encounter_ and all
// its listeners.
class RaidData {
public:
  RaidData(fl::context::AccountCtx ctx, fl::events::RaidBus &events,
           std::uint64_t id, std::span<const fl::monster::MonsterKind> enemies);
  ~RaidData();
  RaidData(const RaidData &) = delete;
  RaidData &operator=(const RaidData &) = delete;
  using ResultClock = std::chrono::steady_clock;
  bool result_expired(ResultClock::time_point now = ResultClock::now()) const {
    return resolved_at_ && result_ != fl::events::RaidResult::Victory &&
           now - *resolved_at_ >= std::chrono::minutes(5);
  }
  std::optional<ResultClock::time_point> resolved_at() const { return resolved_at_; }
  bool active() const noexcept { return !result_; }
  std::optional<fl::events::RaidResult> result() const { return result_; }
  EncounterData &encounter() { return encounter_; }
  const EncounterData &encounter() const { return encounter_; }
  void tick();
  void announce_started();
  std::uint64_t id() const { return id_; }
  std::uint64_t combat_beats() const { return combat_beats_; }

private:
  void resolve(fl::events::RaidResult result);
  void cleanup();
  fl::context::AccountCtx ctx_;
  fl::events::RaidBus &events_;
  std::uint64_t id_;
  entt::entity owner_;
  fl::events::PartyBus combat_bus_;
  EncounterData encounter_;
  std::vector<PartyData *> parties_;
  std::optional<fl::events::RaidResult> result_;
  std::optional<ResultClock::time_point> resolved_at_;
  std::uint64_t combat_beats_{0};
  bool cleaned_{false};
  bool announced_{false};
  fl::events::ScopedPartyListener witness_sub_;
};
} // namespace fl::primitives
