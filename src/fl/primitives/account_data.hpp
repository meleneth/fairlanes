// grand_central.hpp (or better: fl/primitives/account_data.hpp)
#pragma once

#include <deque>
#include <memory>

#include <entt/entt.hpp>

#include "fl/fwd.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "party_data.hpp"
#include "fl/primitives/raid_data.hpp"

namespace fl::primitives {

struct AccountData {
public:
  explicit AccountData(entt::entity account_id)
      : account_id_{account_id},
        log_{std::make_unique<fl::widgets::FancyLog>()} {}

  AccountData(AccountData &&) noexcept = default;
  AccountData &operator=(AccountData &&) noexcept = default;

  AccountData(const AccountData &) = delete;
  AccountData &operator=(const AccountData &) = delete;

  RaidData *raid() { return raid_.get(); }
  const RaidData *raid() const { return raid_.get(); }
  bool in_raid() const { return raid_ && raid_->active(); }
  fl::events::RaidBus &raid_bus() { return *raid_bus_; }
  bool start_raid(fl::context::AccountCtx ctx, std::span<const fl::monster::MonsterKind> enemies) {
    if (in_raid() || parties_.empty() || enemies.empty()) return false;
    raid_.reset();
    raid_ = std::make_unique<RaidData>(ctx, *raid_bus_, ++raid_id_, enemies);
    raid_->announce_started();
    return true;
  }

  // --- capability-style accessors ---
  entt::entity account_id() const { return account_id_; }

  fl::widgets::FancyLog &log() const {
    // FL_ASSERT(log_);
    return *log_;
  }

  std::deque<PartyData> &parties() { return parties_; }
  const std::deque<PartyData> &parties() const { return parties_; }

  PartyData &party(std::size_t idx) { return parties_.at(idx); }
  const PartyData &party(std::size_t idx) const { return parties_.at(idx); }

private:
  entt::entity account_id_{entt::null};
  std::unique_ptr<fl::widgets::FancyLog> log_{};

  std::deque<PartyData> parties_{}; // owned parties
  std::unique_ptr<fl::events::RaidBus> raid_bus_{std::make_unique<fl::events::RaidBus>()};
  std::uint64_t raid_id_{0};
  std::unique_ptr<RaidData> raid_; // destroyed before participants and event bus
};

} // namespace fl::primitives
