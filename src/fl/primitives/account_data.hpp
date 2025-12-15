// grand_central.hpp
#pragma once

#include <deque>
#include <entt/entt.hpp>

#include "fl/events/account_bus.hpp"
#include "fl/fwd.hpp"
#include "fl/primitives/fancy_log_sink.hpp"
#include "party_data.hpp"

/*
#include "fl/widgets/fancy_log.hpp"
#include "logging.hpp"
#include "random_hub.hpp"
*/

namespace fl::primitives {

struct AccountData {
  entt::entity account_id_;
  std::shared_ptr<fl::widgets::FancyLog> log_;
  fl::events::AccountBus bus_;    // per-account event bus
  std::deque<PartyData> parties_; // owned parties

  AccountData(entt::entity account_id);

  AccountData(AccountData &&) noexcept = default;
  AccountData &operator=(AccountData &&) noexcept = default;

  AccountData(const AccountData &) = delete;
  AccountData &operator=(const AccountData &) = delete;
};

} // namespace fl::primitives
