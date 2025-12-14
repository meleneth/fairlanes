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
  Tick,       // Party loop iteration
  PreAttack,  // before an entity attacks
  PostAttack, // after an entity attacks
              // Add more as necessary
};

/// Payloads vary per event; simplest is to use std::any.
/// You can switch to variant or strongly-typed event structs later.
using PartyPayload = std::any;

/// The PartyBus: a scoped event dispatcher for a single Party.
using PartyBus =
    eventpp::EventDispatcher<PartyEvent, void(const PartyPayload &)>;

} // namespace fl::events
