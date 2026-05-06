#pragma once

#include <concepts>
#include <entt/entt.hpp>

#include "fl/events/account_bus.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/fwd.hpp" // if this already forward-declares some of these
#include "fl/primitives/damage.hpp"

namespace fl::primitives {
class RandomHub;
struct AccountData;
struct PartyData;
struct Damage; // only if you KEEP it as value you must include its header
} // namespace fl::primitives

namespace fl::widgets {
class FancyLog;
}

namespace fl::context {

struct EntityCtx {
public:
  EntityCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
            fl::widgets::FancyLog &log, entt::entity self);

  EntityCtx(const EntityCtx &) = default;
  EntityCtx &operator=(const EntityCtx &rhs);

  entt::registry &reg() const {
    //  FL_ASSERT(reg_);
    return *reg_;
  }
  fl::primitives::RandomHub &rng() const {
    // FL_ASSERT(rng_);
    return *rng_;
  }
  fl::widgets::FancyLog &log() const {
    // FL_ASSERT(log_);
    return *log_;
  }
  entt::entity self() const { return self_; }

  EntityCtx entity_context(entt::entity ent) const;
  [[nodiscard]] PartyCtx expect_party_ctx() const;

private:
  entt::registry *reg_{};
  fl::primitives::RandomHub *rng_{};
  fl::widgets::FancyLog *log_{};
  entt::entity self_{entt::null};
};

struct BuildCtx {
public:
  BuildCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
           fl::widgets::FancyLog &log)
      : reg_{&reg}, rng_{&rng}, log_{&log} {}

  BuildCtx() = delete;

  entt::registry &reg() const { return *reg_; }

  fl::primitives::RandomHub &rng() const { return *rng_; }

  fl::widgets::FancyLog &log() const { return *log_; }

  EntityCtx entity_context(entt::entity ent) const;

private:
  entt::registry *reg_{};
  fl::primitives::RandomHub *rng_{};
  fl::widgets::FancyLog *log_{};
};

struct PartyCtx {
public:
  PartyCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
           fl::primitives::AccountData &acc, fl::primitives::PartyData &party);

  PartyCtx(const PartyCtx &) = default;
  PartyCtx &operator=(const PartyCtx &) = default;

  entt::registry &reg() const {
    // FL_ASSERT(reg_);
    return *reg_;
  }
  fl::primitives::RandomHub &rng() const {
    // FL_ASSERT(rng_);
    return *rng_;
  }
  fl::primitives::AccountData &account_data() const {
    // FL_ASSERT(account_data_);
    return *account_data_;
  }
  fl::primitives::PartyData &party_data() const {
    // FL_ASSERT(party_data_);
    return *party_data_;
  }
  fl::widgets::FancyLog &log() const {
    // FL_ASSERT(log_);
    return *log_;
  }
  fl::events::PartyBus &bus() const {
    //  FL_ASSERT(bus_);
    return *bus_;
  }

  entt::entity self() const;
  EntityCtx entity_context(entt::entity ent) const;
  BuildCtx build_context() const;

private:
  entt::registry *reg_{};
  fl::primitives::RandomHub *rng_{};
  fl::primitives::AccountData *account_data_{};
  fl::primitives::PartyData *party_data_{};
  fl::widgets::FancyLog *log_{};
  fl::events::PartyBus *bus_{};
};

struct AccountCtx {
public:
  entt::registry &reg() const {
    // FL_ASSERT(reg_);
    return *reg_;
  }
  fl::primitives::RandomHub &rng() const {
    // FL_ASSERT(rng_);
    return *rng_;
  }
  fl::primitives::AccountData &account_data() const {
    // FL_ASSERT(account_);
    return *account_data_;
  }
  fl::widgets::FancyLog &log() const;
  fl::events::AccountBus &bus() const;

  entt::entity self() const;
  
  PartyCtx party_context(std::size_t idx) const;
  PartyCtx party_context(fl::primitives::PartyData &data) const;
  EntityCtx entity_context(entt::entity ent) const;
  AccountCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
             fl::primitives::AccountData &account)
      : reg_{&reg}, rng_{&rng}, account_data_{&account} {}

private:
  entt::registry *reg_{};
  fl::primitives::RandomHub *rng_{};
  fl::primitives::AccountData *account_data_{};
  // TODO this has to be broken
  // fl::widgets::FancyLog *log_{};
  // fl::events::AccountBus *bus_{};
};

struct AttackCtx {
public:
  AttackCtx(entt::registry &reg, fl::primitives::RandomHub &rng,
            fl::widgets::FancyLog &log, entt::entity attacker,
            entt::entity defender);

  entt::registry &reg() const {
    // FL_ASSERT(reg_);
    return *reg_;
  }

  fl::primitives::RandomHub &rng() const {
    // FL_ASSERT(rng_);
    return *rng_;
  }

  fl::widgets::FancyLog &log() const {
    // FL_ASSERT(log_);
    return *log_;
  }

  entt::entity attacker() const {
    // FL_ASSERT(attacker_ != entt::null);
    return attacker_;
  }

  entt::entity defender() const {
    // FL_ASSERT(defender_ != entt::null);
    return defender_;
  }

  fl::primitives::Damage &damage() { return damage_; }

  const fl::primitives::Damage &damage() const { return damage_; }

  fl::context::EntityCtx entity_context(entt::entity e) const;
  static AttackCtx make_attack(PartyCtx &ctx, entt::entity attacker,
                               entt::entity defender);

private:
  entt::registry *reg_{};
  fl::primitives::RandomHub *rng_{};
  fl::widgets::FancyLog *log_{};
  entt::entity attacker_{entt::null};
  entt::entity defender_{entt::null};
  fl::primitives::Damage
      damage_; // if you want to forward-declare Damage, make this a pointer too
};

template <typename C>
concept WorldCoreCtx = requires(C c) {
  { c.reg() } -> std::same_as<entt::registry &>;
  { c.log() } -> std::same_as<fl::widgets::FancyLog &>;
  { c.rng() } -> std::same_as<fl::primitives::RandomHub &>;
};

} // namespace fl::context
