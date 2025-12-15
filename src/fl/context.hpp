#pragma once

#include <concepts>

#include <entt/entt.hpp>

#include "fl/events/account_bus.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/fsm/party_loop_ctx.hpp"
#include "fl/fwd.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/damage.hpp"

namespace fl::context {

struct EntityCtx {
  entt::registry &reg_;
  fl::primitives::RandomHub &rng_;
  fl::widgets::FancyLog &log_;
  entt::entity self_;

  EntityCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
            fl::widgets::FancyLog &log, entt::entity self);

  EntityCtx(const EntityCtx &) = default;

  entt::registry &reg() const { return reg_; }

  EntityCtx &operator=(const EntityCtx &rhs);

  EntityCtx entity_context(entt::entity ent) const;
};

struct BuildCtx {
  entt::registry &reg_;
  fl::primitives::RandomHub &rng_;
  fl::widgets::FancyLog &log_;

  entt::registry &reg() const { return reg_; }
  EntityCtx entity_context(entt::entity ent) const;
};

struct PartyCtx {
  entt::registry &reg_;
  fl::primitives::RandomHub &rng_;

  fl::primitives::AccountData &account_data_;
  fl::primitives::PartyData *party_data_;

  fl::widgets::FancyLog &log_;
  fl::events::PartyBus &bus_;
  entt::registry &reg() const { return reg_; }
  fl::widgets::FancyLog &log() const { return log_; }
  entt::entity self_() const;

  PartyCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
           fl::primitives::AccountData &acc, fl::primitives::PartyData &party);
  PartyCtx &operator=(const PartyCtx &rhs);

  EntityCtx entity_context(entt::entity ent) const;
  fl::fsm::PartyLoopCtx party_loop_context();
  BuildCtx build_context() const;
};

struct AccountCtx {
  entt::registry &reg_;
  fl::primitives::RandomHub &rng_;
  fl::primitives::AccountData &account_;

  fl::widgets::FancyLog &log() const;
  fl::events::AccountBus &bus() const;

  PartyCtx party_context(std::size_t idx) const;
  PartyCtx party_context(fl::primitives::PartyData &data) const;
  entt::registry &reg() const { return reg_; }
  EntityCtx entity_context(entt::entity ent) const;
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
            entt::entity defender);

  fl::context::EntityCtx entity_context(entt::entity e) const;
  entt::registry &reg() const { return reg_; }
  static AttackCtx make_attack(PartyCtx &ctx, entt::entity attacker,
                               entt::entity defender);

  // MARK_CLASS_M OVEONLY(AttackCtx);
};

template <typename C>
concept WorldCoreCtx = requires(C c) {
  { c.reg() } -> std::same_as<entt::registry &>;
  { c.log() } -> std::same_as<fl::widgets::FancyLog &>;
  { c.rng() } -> std::same_as<fl::primitives::RandomHub &>;
};

} // namespace fl::context
