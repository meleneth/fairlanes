#include <entt/entt.hpp>

#include "context.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"

/*
#include "fl/widgets/fancy_log.hpp"
*/
namespace fl::context {

EntityCtx::EntityCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
                     fl::widgets::FancyLog &log, entt::entity self)
    : reg_(reg), rng_(rng), log_(log), self_(self) {}

EntityCtx &EntityCtx::operator=(const EntityCtx &rhs) {
  self_ = rhs.self_;
  return *this;
}

EntityCtx EntityCtx::entity_context(entt::entity ent) const {
  return EntityCtx{reg(), rng_, log_, ent};
}

EntityCtx BuildCtx::entity_context(entt::entity ent) const {
  return EntityCtx{reg(), rng_, log_, ent};
}

entt::entity PartyCtx::self_() const { return party_data_->party_id_; }

PartyCtx::PartyCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
                   fl::primitives::AccountData &acc,
                   fl::primitives::PartyData &party)
    : reg_(reg), rng_(rng), account_data_(acc), party_data_(&party),
      log_(*party.log_), bus_(party.party_bus_) {}

EntityCtx PartyCtx::entity_context(entt::entity ent) const {
  return EntityCtx{reg(), rng_, *party_data_->log_, ent};
}

BuildCtx PartyCtx::build_context() const {
  return BuildCtx{reg(), rng_, *party_data_->log_};
}

fl::fsm::PartyLoopCtx PartyCtx::party_loop_context() {
  return fl::fsm::PartyLoopCtx{*this};
}

fl::widgets::FancyLog &AccountCtx::log() const { return *account_.log_; }
fl::events::AccountBus &AccountCtx::bus() const { return account_.bus_; }

PartyCtx AccountCtx::party_context(std::size_t idx) const {
  return PartyCtx{reg(), rng_, account_, account_.parties_.at(idx)};
}

PartyCtx AccountCtx::party_context(fl::primitives::PartyData &data) const {
  return PartyCtx{reg(), rng_, account_, data};
}

EntityCtx AccountCtx::entity_context(entt::entity ent) const {
  return EntityCtx{reg(), rng_, *account_.log_, ent};
}

PartyCtx &PartyCtx::operator=(const PartyCtx &rhs) {
  party_data_ = rhs.party_data_;
  return *this;
}
AttackCtx::AttackCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
                     fl::widgets::FancyLog &log, entt::entity attacker,
                     entt::entity defender)
    : reg_(reg), rng_(rng), log_(log), attacker_(attacker),
      defender_(defender) {}

EntityCtx AttackCtx::entity_context(entt::entity e) const {
  return EntityCtx{reg(), rng_, log_, e};
}

AttackCtx AttackCtx::make_attack(PartyCtx &ctx, entt::entity attacker,
                                 entt::entity defender) {
  return AttackCtx{ctx.reg(), ctx.rng_, ctx.log_, attacker, defender};
}

// MARK_CLASS_M OVEONLY(AttackCtx);
} // namespace fl::context
