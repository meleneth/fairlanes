#include <entt/entt.hpp>

#include "context.hpp"

#include "fl/context.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"

// If FancyLog is only forward-declared in context.hpp, you may need this here:
// #include "fl/widgets/fancy_log.hpp"

namespace fl::context {

fl::widgets::FancyLog &AccountCtx::log() const {
  // FL_ASSERT(log_);
  return account_data_->log();
}
fl::events::AccountBus &AccountCtx::bus() const {
  // FL_ASSERT(bus_);
  return account_data_->bus();
}

EntityCtx::EntityCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
                     fl::widgets::FancyLog &log, entt::entity self)
    : reg_(&reg), rng_(&rng), log_(&log), self_(self) {}

EntityCtx &EntityCtx::operator=(const EntityCtx &rhs) {
  reg_ = rhs.reg_;
  rng_ = rhs.rng_;
  log_ = rhs.log_;
  self_ = rhs.self_;
  return *this;
}

EntityCtx EntityCtx::entity_context(entt::entity ent) const {
  return EntityCtx{reg(), rng(), log(), ent};
}

EntityCtx BuildCtx::entity_context(entt::entity ent) const {
  return EntityCtx{reg(), rng(), log(), ent};
}

entt::entity PartyCtx::self() const { return party_data().party_id(); }

PartyCtx::PartyCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
                   fl::primitives::AccountData &acc,
                   fl::primitives::PartyData &party)
    : reg_(&reg), rng_(&rng), account_data_(&acc), party_data_(&party),
      log_(&party.log()), // assumes PartyData stores FancyLog* log_
      bus_(&party.party_bus()) {
} // assumes PartyData stores PartyBus party_bus_

PartyCtx &PartyCtx::operator=(const PartyCtx &rhs) {
  reg_ = rhs.reg_;
  rng_ = rhs.rng_;
  account_data_ = rhs.account_data_;
  party_data_ = rhs.party_data_;
  log_ = rhs.log_;
  bus_ = rhs.bus_;
  return *this;
}

EntityCtx PartyCtx::entity_context(entt::entity ent) const {
  return EntityCtx{reg(), rng(), log(), ent};
}

BuildCtx PartyCtx::build_context() const {
  return BuildCtx{reg(), rng(), log()};
}

PartyCtx AccountCtx::party_context(std::size_t idx) const {
  return PartyCtx{reg(), rng(), account_data(),
                  account_data().parties().at(idx)};
}

PartyCtx AccountCtx::party_context(fl::primitives::PartyData &data) const {
  return PartyCtx{reg(), rng(), account_data(), data};
}

EntityCtx AccountCtx::entity_context(entt::entity ent) const {
  return EntityCtx{reg(), rng(), log(), ent};
}

entt::entity AccountCtx::self() const {
  return account_data_->account_id();
}


AttackCtx::AttackCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
                     fl::widgets::FancyLog &log, entt::entity attacker,
                     entt::entity defender)
    : reg_(&reg), rng_(&rng), log_(&log), attacker_(attacker),
      defender_(defender) {}

EntityCtx AttackCtx::entity_context(entt::entity e) const {
  return EntityCtx{reg(), rng(), log(), e};
}

AttackCtx AttackCtx::make_attack(PartyCtx &ctx, entt::entity attacker,
                                 entt::entity defender) {
  return AttackCtx{ctx.reg(), ctx.rng(), ctx.log(), attacker, defender};
}

// MARK_CLASS_MOVEONLY(AttackCtx);

} // namespace fl::context
