#pragma once

#include <cstdint>
#include <eventpp/eventdispatcher.h>
#include <string>
#include <variant>


namespace fl {

//
// -----------------------------------------------------------------------------
// Event Type Identifiers (uint32_t or enum)
// -----------------------------------------------------------------------------
// Using integers keeps EventPP fast and avoids template explosion.
// You can use scoped enums if you prefer, but integers are most flexible.
//
enum class EventType : std::uint32_t {
  Combat = 1,
  Log = 2,
  Account = 3,
  Party = 4,
  Encounter = 5,
};

//
// -----------------------------------------------------------------------------
// Event Payload Variants
// -----------------------------------------------------------------------------
// For now, keep them simple. You'll expand these with structs as gameplay
// grows. Example events:
//
//   - CombatEvent{attacker, defender, damage}
//   - LogEvent{string}
//   - AccountEvent{account_id}
//   - PartyEvent{party_id}
//   - EncounterTick{dt}
//
// You can grow this as the game evolves.
//
struct LogEvent {
  std::string text;
};

// Example combat event that systems can react to.
struct CombatEvent {
  std::uint32_t attacker;
  std::uint32_t defender;
  int damage;
};

// Generic time event for encounters
struct TickEvent {
  int ms;
};

using EventPayload = std::variant<LogEvent, CombatEvent, TickEvent>;

//
// -----------------------------------------------------------------------------
// Dispatcher typedef
// -----------------------------------------------------------------------------
// eventpp::EventDispatcher key = uint32_t (EventType), payload = EventPayload
//
using EventBus =
    eventpp::EventDispatcher<std::uint32_t, // event type (EventType cast to
                                            // uint32_t)
                             void(const EventPayload &) // callback signature
                             >;

//
// -----------------------------------------------------------------------------
// Specialized buses
// -----------------------------------------------------------------------------
// These are conceptual distinctions. They all use EventBus underneath,
// but representing them as distinct types creates separation of concerns.
//
struct AccountBus : public EventBus {};
struct PartyBus : public EventBus {};
struct EncounterBus : public EventBus {};

//
// -----------------------------------------------------------------------------
// Convenience helpers
// -----------------------------------------------------------------------------

inline std::uint32_t to_key(EventType t) {
  return static_cast<std::uint32_t>(t);
}

} // namespace fl
