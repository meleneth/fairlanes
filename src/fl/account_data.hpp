// grand_central.hpp
#pragma once

#include <deque>
#include <entt/entt.hpp>

#include "fl/bus_types.hpp"
#include "fl/party_data.hpp"
#include "fl/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl {

struct AccountData {
  entt::entity account_entity{entt::null};
  widgets::FancyLog log;             // per-account log
  AccountBus bus;                    // per-account event bus
  std::deque<fl::PartyData> parties; // owned parties

  AccountData() = default;

  AccountData(AccountData &&) noexcept = default;
  AccountData &operator=(AccountData &&) noexcept = default;

  AccountData(const AccountData &) = delete;
  AccountData &operator=(const AccountData &) = delete;
};

} // namespace fl
