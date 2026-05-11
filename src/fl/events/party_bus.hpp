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
struct PartyGainedXP {
  int amount{0};
};
struct PartyHealed {
  entt::entity member{entt::null};
  int amount{0};
};
struct PartyTick {};
struct PreAttack {
  entt::entity attacker{entt::null};
  entt::entity target{entt::null};
};
struct PostAttack {
  entt::entity attacker{entt::null};
  entt::entity target{entt::null};
  int damage{0};
};
struct PlayerDied {
  entt::entity player{entt::null};
  entt::entity killer{entt::null};
};

using PartyEvent =
    std::variant<PartyCreated, MemberJoined, MemberLeft, PartyWiped,
         PartyGainedXP, PartyHealed, PartyTick, PreAttack, PostAttack,
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
