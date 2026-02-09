// grand_central.hpp (or better: fl/primitives/account_data.hpp)
#pragma once

#include <deque>
#include <memory>

#include <entt/entt.hpp>

#include "fl/events/account_bus.hpp"
#include "fl/fwd.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "party_data.hpp"

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

  // --- capability-style accessors ---
  entt::entity account_id() const { return account_id_; }

  fl::widgets::FancyLog &log() const {
    // FL_ASSERT(log_);
    return *log_;
  }

  fl::events::AccountBus &bus() { return bus_; }
  const fl::events::AccountBus &bus() const { return bus_; }

  std::deque<PartyData> &parties() { return parties_; }
  const std::deque<PartyData> &parties() const { return parties_; }

  PartyData &party(std::size_t idx) { return parties_.at(idx); }
  const PartyData &party(std::size_t idx) const { return parties_.at(idx); }

private:
  entt::entity account_id_{entt::null};
  std::unique_ptr<fl::widgets::FancyLog> log_{};

  fl::events::AccountBus bus_{};    // per-account event bus
  std::deque<PartyData> parties_{}; // owned parties
};

} // namespace fl::primitives
