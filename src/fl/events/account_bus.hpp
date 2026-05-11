#pragma once

#include <entt/entt.hpp>

#include <variant>

#include "sr/variant_bus.hpp"

namespace fl::events {

struct AccountCreated {
  entt::entity account{entt::null};
  entt::entity related{entt::null};
};

struct AccountDeleted {
  entt::entity account{entt::null};
  entt::entity related{entt::null};
};

struct AccountRenamed {
  entt::entity account{entt::null};
  entt::entity related{entt::null};
};

struct AccountTick {
  entt::entity account{entt::null};
};

using AccountEvent =
    std::variant<AccountCreated, AccountDeleted, AccountRenamed, AccountTick>;

using AccountBus = seerin::VariantBus<AccountEvent>;

} // namespace fl::events
