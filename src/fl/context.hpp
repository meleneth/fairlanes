#pragma once

#include <entt/entt.hpp>

#include "fl/events/account_bus.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::context {

struct EntityCtx {
  entt::registry &reg_;
  fl::primitives::RandomHub &rng_;
  fl::widgets::FancyLog &log_;
  entt::entity self_;

  EntityCtx entity_context(entt::entity ent) const {
    return EntityCtx{reg_, rng_, log_, ent};
  }
};

struct PartyCtx {
  entt::registry &reg_;
  fl::primitives::RandomHub &rng_;

  fl::primitives::AccountData &acc_;
  fl::primitives::PartyData &party_;

  fl::widgets::FancyLog &log() const { return *party_.log_; }
  fl::primitives::PartyBus &bus() const { return party_.bus_; }

  entt::entity self_() const { return party_.party_id_; }

  EntityCtx entity_context(entt::entity ent) const {
    return EntityCtx{reg_, rng_, *party_.log_, ent};
  }
};

struct AccountCtx {
  entt::registry &reg_;
  fl::primitives::RandomHub &rng_;
  fl::primitives::AccountData &account_;

  fl::widgets::FancyLog &log() const { return *account_.log_; }
  fl::events::AccountBus &bus() const { return account_.bus_; }

  PartyCtx party_context(std::size_t idx) const {
    return PartyCtx{reg_, rng_, account_, account_.parties_.at(idx)};
  }

  EntityCtx entity_context(entt::entity ent) const {
    return EntityCtx{reg_, rng_, *account_.log_, ent};
  }
};

} // namespace fl::context
