// grand_central.hpp
#pragma once

#include <deque>
#include <entt/entt.hpp>

#include "fl/events/account_bus.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "logging.hpp"
#include "party_data.hpp"
#include "random_hub.hpp"

namespace fl::primitives {

struct AccountData {
  entt::entity account_entity_{entt::null};
  widgets::FancyLog log_;         // per-account log
  fl::events::AccountBus bus_;    // per-account event bus
  std::deque<PartyData> parties_; // owned parties

  AccountData() = default;

  AccountData(AccountData &&) noexcept = default;
  AccountData &operator=(AccountData &&) noexcept = default;

  AccountData(const AccountData &) = delete;
  AccountData &operator=(const AccountData &) = delete;
};

} // namespace fl::primitives
