#pragma once
// encounter_events.hpp

#include <chrono>
#include <cstdint>
#include <variant>
#include <vector>

namespace seerin::enc {

using ns = std::chrono::nanoseconds;
using CombatantId = std::uint32_t;

// External input to encounter
struct Beat {
  ns dt{0};
};

struct CommandChosen {
  CombatantId who{};
  ns action_time{0};
  // Optional future: token/target/skill ids live here (still tiny + copyable).
};

struct ApplyStun {
  CombatantId who{};
  ns duration{0};
};
struct Kill {
  CombatantId who{};
};
struct Revive {
  CombatantId who{};
  std::int64_t initial_charge_units{0};
};

using InEvent = std::variant<Beat, CommandChosen, ApplyStun, Kill, Revive>;

// Encounter outputs (to UI/rules/trace)
struct NeedCommand {
  CombatantId who{};
};
struct ReadyQueueChanged {
  std::vector<CombatantId> order;
};
struct ActiveChanged {
  CombatantId who{};
  bool is_active{false};
};
struct ActionResolved {
  CombatantId who{};
};

// Optional trace
struct Trace {
  CombatantId who{};
  std::uint8_t from{};
  std::uint8_t to{};
};

using OutEvent = std::variant<NeedCommand, ReadyQueueChanged, ActiveChanged,
                              ActionResolved, Trace>;

} // namespace seerin::enc
