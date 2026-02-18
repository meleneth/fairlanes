#pragma once

#include <any>

#include <eventpp/eventdispatcher.h>

namespace fl::events {

/// Events that occur within a specific Party.
///
/// These are intentionally high-level and gameplay-relevant.
/// Combat animations, skill triggers, damage application,
/// party-wide XP grants, etc.
enum class PartyEvent {
  PartyCreated,
  MemberJoined,
  MemberLeft,
  PartyWiped,
  PartyGainedXP,
  PartyHealed,
  Tick,
  PreAttack,
  PostAttack,
};

using PartyPayload = std::any;

using PartyBus =
    eventpp::EventDispatcher<PartyEvent, void(const PartyPayload &)>;
using PartyListenerHandle =
    decltype(std::declval<fl::events::PartyBus &>().appendListener(
        fl::events::PartyEvent::Tick,
        std::function<void(fl::events::PartyPayload const &)>{}));

struct ScopedPartyListener {
  fl::events::PartyBus *bus_{nullptr};
  fl::events::PartyEvent event_{};
  decltype(std::declval<fl::events::PartyBus &>().appendListener(
      fl::events::PartyEvent::Tick,
      std::function<void(fl::events::PartyPayload const &)>{})) handle_{};
  bool connected_{false};

  ScopedPartyListener() = default;

  template <class Fn>
  ScopedPartyListener(fl::events::PartyBus &bus, fl::events::PartyEvent event,
                      Fn &&fn)
      : bus_(&bus), event_(event) {
    handle_ = bus_->appendListener(event_, std::forward<Fn>(fn));
    connected_ = true;
  }

  ScopedPartyListener(ScopedPartyListener &&rhs) noexcept {
    *this = std::move(rhs);
  }
  ScopedPartyListener &operator=(ScopedPartyListener &&rhs) noexcept {
    if (this == &rhs)
      return *this;
    reset();
    bus_ = rhs.bus_;
    event_ = rhs.event_;
    handle_ = rhs.handle_;
    connected_ = rhs.connected_;
    rhs.bus_ = nullptr;
    rhs.connected_ = false;
    return *this;
  }

  ~ScopedPartyListener() { reset(); }

  void reset() {
    if (connected_ && bus_) {
      bus_->removeListener(event_, handle_);
    }
    connected_ = false;
    bus_ = nullptr;
  }

  ScopedPartyListener(const ScopedPartyListener &) = delete;
  ScopedPartyListener &operator=(const ScopedPartyListener &) = delete;
};

} // namespace fl::events
