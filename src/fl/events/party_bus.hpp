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

} // namespace fl::events
