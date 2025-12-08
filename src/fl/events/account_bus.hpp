#pragma once

#include <entt/entt.hpp>
#include <eventpp/eventdispatcher.h>

namespace fl::events {

/// High-level account lifecycle / meta events.
enum class AccountEventId {
  Created,
  Deleted,
  Renamed,
  Tick, // per-account update tick, if you want it
  // Add more as needed
};

struct AccountEvent {
  AccountEventId type;
  entt::entity account;              // the account entity in the registry
  entt::entity related = entt::null; // optional: party, encounter, etc
};

/// AccountBus: immediate-dispatch event bus for account-level events.
///
/// Usage:
///   fl::events::AccountBus bus;
///   bus.appendListener(fl::events::AccountEventId::Created,
///     [](const fl::events::AccountEvent & ev) { ... });
///
using AccountBus =
    eventpp::EventDispatcher<AccountEventId, void(const AccountEvent &)>;

} // namespace fl::events
