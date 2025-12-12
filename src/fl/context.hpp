#pragma once

#include <entt/entt.hpp>

#include "fl/events/account_bus.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/damage.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::context {

struct EntityCtx {
  entt::registry &reg_;
  fl::primitives::RandomHub &rng_;
  fl::widgets::FancyLog &log_;
  entt::entity self_;

  EntityCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
            fl::widgets::FancyLog &log, entt::entity self)
      : reg_(reg), rng_(rng), log_(log), self_(self) {}

  EntityCtx(const EntityCtx &) = default;

  EntityCtx &operator=(const EntityCtx &rhs) {
    self_ = rhs.self_;
    return *this;
  }

  EntityCtx entity_context(entt::entity ent) const {
    return EntityCtx{reg_, rng_, log_, ent};
  }
};

struct BuildCtx {
  entt::registry &reg_;
  fl::primitives::RandomHub &rng_;
  fl::widgets::FancyLog &log_;

  EntityCtx entity_context(entt::entity ent) const {
    return EntityCtx{reg_, rng_, log_, ent};
  }
};

struct PartyCtx {
  entt::registry &reg_;
  fl::primitives::RandomHub &rng_;

  fl::primitives::AccountData &acc_;
  fl::primitives::PartyData &party_;

  fl::widgets::FancyLog &log_;
  fl::primitives::PartyBus &bus_;

  entt::entity self_() const { return party_.party_id_; }

  PartyCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
           fl::primitives::AccountData &acc, fl::primitives::PartyData &party)
      : reg_(reg), rng_(rng), acc_(acc), party_(party), log_(*party.log_),
        bus_(party.bus_) {}

  EntityCtx entity_context(entt::entity ent) const {
    return EntityCtx{reg_, rng_, *party_.log_, ent};
  }

  BuildCtx build_context() const { return BuildCtx{reg_, rng_, *party_.log_}; }
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

struct AttackCtx {
  entt::registry &reg_;
  fl::primitives::RandomHub &rng_;
  fl::widgets::FancyLog &log_;
  entt::entity attacker_;
  entt::entity defender_;
  fl::primitives::Damage damage_;

  AttackCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
            fl::widgets::FancyLog &log, entt::entity attacker,
            entt::entity defender)
      : reg_(reg), rng_(rng), log_(log), attacker_(attacker),
        defender_(defender) {}

  fl::context::EntityCtx entity_context(entt::entity e) const {
    return fl::context::EntityCtx{reg_, rng_, log_, e};
  }

  static AttackCtx make_attack(PartyCtx &ctx, entt::entity attacker,
                               entt::entity defender) {
    return AttackCtx{ctx.reg_, ctx.rng_, ctx.log_, attacker, defender};
  }

  // MARK_CLASS_M OVEONLY(AttackCtx);
};

template <typename C>
concept WorldCoreCtx = requires(C c) {
  { c.reg_ } -> std::same_as<entt::registry &>;
  { c.log_ } -> std::same_as<fl::widgets::FancyLog &>;
  { c.rng_ } -> std::same_as<fl::primitives::RandomHub &>;
};

} // namespace fl::context
