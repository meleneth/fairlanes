#pragma once
// atb_events.hpp

#include <chrono>
#include <cstdint>
#include <variant>

namespace seerin::atb {

using ns = std::chrono::nanoseconds;

struct Beat {
  ns dt{0};
};

struct CommandSelected {
  ns action_time{0};
};
struct StunApplied {
  ns duration{0};
};
struct Killed {};
struct Revived {
  std::int64_t initial_charge_units{0};
};

using Event = std::variant<Beat, CommandSelected, StunApplied, Killed, Revived>;

// Output domain (also id-less)
enum class StateTag : std::uint8_t {
  Charging,
  Ready,
  Acting,
  ChargingStunned,
  ReadyStunned,
  ActingStunned,
  Dead,
};

struct BecameReady {};
struct ActionStarted {
  ns action_time{0};
};
struct ActionFinished {};
struct StateChanged {
  StateTag from;
  StateTag to;
};

using OutputEvent =
    std::variant<BecameReady, ActionStarted, ActionFinished, StateChanged>;

} // namespace seerin::atb
