#pragma once

#include <entt/entt.hpp>

#include "fl/account_data.hpp"
#include "fl/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "party_data.hpp"

namespace fl::context {

struct EntityCtx {
  entt::registry &reg;
  fl::RandomHub &rng;
  fl::widgets::FancyLog &log;
  entt::entity self;

  EntityCtx entity_context(entt::entity ent) const {
    return EntityCtx{reg, rng, log, ent};
  }
};

struct PartyCtx {
  entt::registry &reg;
  fl::RandomHub &rng;
  fl::AccountData &acc;
  fl::PartyData &party;

  fl::widgets::FancyLog &log() const { return party.log_; }
  fl::PartyBus &bus() const { return party.bus_; }

  EntityCtx entity_context(entt::entity ent) const {
    return EntityCtx{reg, rng, party.log_, ent};
  }
};

struct AccountCtx {
  entt::registry &reg;
  fl::RandomHub &rng;
  fl::AccountData &account;

  fl::widgets::FancyLog &log() const { return account.log; }
  fl::AccountBus &bus() const { return account.bus; }

  PartyCtx party_context(std::size_t idx) const {
    return PartyCtx{reg, rng, account, account.parties.at(idx)};
  }

  EntityCtx entity_context(entt::entity ent) const {
    return EntityCtx{reg, rng, account.log, ent};
  }
};

} // namespace fl::context
