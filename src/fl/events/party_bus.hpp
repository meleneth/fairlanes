#pragma once

#include <entt/entt.hpp>

#include <functional>
#include <utility>
#include <variant>

#include "sr/variant_bus.hpp"

namespace fl::events {

struct PartyCreated {};
struct MemberJoined {
  entt::entity member{entt::null};
};
struct MemberLeft {
  entt::entity member{entt::null};
};
struct PartyWiped {};
struct PartyVictory {};
struct PartyLeftCombat {};
struct PartyGainedXP {
  int amount{0};
};
struct PartyGainedLevel {
  entt::entity member{entt::null};
  int level{1};
};
struct PartyHealed {
  entt::entity member{entt::null};
  int amount{0};
};
struct PartyRevitalizeRequested {};
struct LootDropRequested {
  entt::entity source{entt::null};
  entt::entity party{entt::null};
};
struct PartyTick {};
struct PoisonApplied {
  entt::entity source{entt::null};
  entt::entity target{entt::null};
  int damage_per_tick{0};
  int duration_seconds{0};
};
struct FreezeApplied {
  entt::entity source{entt::null};
  entt::entity target{entt::null};
  int duration_seconds{0};
};
struct FreezeStarted {
  entt::entity target{entt::null};
};
struct FreezeEnded {
  entt::entity target{entt::null};
};
struct PreAttack {
  entt::entity attacker{entt::null};
  entt::entity target{entt::null};
};
struct PostAttack {
  entt::entity attacker{entt::null};
  entt::entity target{entt::null};
  int damage{0};
};
struct FleeAttempted {
  entt::entity source{entt::null};
  int chance_percent{0};
  int roll{0};
  bool success{false};
};
struct CombatantFled {
  entt::entity source{entt::null};
};
struct PlayerDied {
  entt::entity player{entt::null};
  entt::entity killer{entt::null};
};

using PartyEvent =
  std::variant<PartyCreated, MemberJoined, MemberLeft, PartyWiped,
         PartyVictory,
                 PartyLeftCombat, PartyGainedXP, PartyGainedLevel, PartyHealed,
                 PartyRevitalizeRequested, LootDropRequested, PartyTick,
                 PoisonApplied, FreezeApplied, FreezeStarted, FreezeEnded,
                 PreAttack, PostAttack, FleeAttempted, CombatantFled,
                 PlayerDied>;

using PartyBus = seerin::VariantBus<PartyEvent>;

struct ScopedPartyListener {
  std::function<void()> disconnect_;
  bool connected_{false};

  ScopedPartyListener() = default;

  template <class T, class Fn>
  ScopedPartyListener(fl::events::PartyBus &bus, std::in_place_type_t<T>,
                      Fn &&fn) {
    auto handle = bus.template on<T>(std::forward<Fn>(fn));
    disconnect_ = [&bus, handle]() mutable {
      bus.template callbacks<T>().remove(handle);
    };
    connected_ = true;
  }

  ScopedPartyListener(ScopedPartyListener &&rhs) noexcept {
    *this = std::move(rhs);
  }
  ScopedPartyListener &operator=(ScopedPartyListener &&rhs) noexcept {
    if (this == &rhs)
      return *this;
    reset();
    disconnect_ = std::move(rhs.disconnect_);
    connected_ = rhs.connected_;
    rhs.connected_ = false;
    return *this;
  }

  ~ScopedPartyListener() { reset(); }

  void reset() {
    if (connected_ && disconnect_) {
      disconnect_();
    }
    connected_ = false;
    disconnect_ = nullptr;
  }

  ScopedPartyListener(const ScopedPartyListener &) = delete;
  ScopedPartyListener &operator=(const ScopedPartyListener &) = delete;
};

} // namespace fl::events
