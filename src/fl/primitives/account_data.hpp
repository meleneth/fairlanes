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
  entt::entity account_id_{entt::null};
  std::shared_ptr<fl::widgets::FancyLog> log_;
  fl::events::AccountBus bus_;     // per-account event bus
  std::vector<PartyData> parties_; // owned parties

  AccountData(entt::entity account_id)
      : account_id_(account_id),
        log_(std::make_shared<fl::widgets::FancyLog>()) {}

  AccountData(AccountData &&) noexcept = default;
  AccountData &operator=(AccountData &&) noexcept = default;

  AccountData(const AccountData &) = delete;
  AccountData &operator=(const AccountData &) = delete;
};

} // namespace fl::primitives
