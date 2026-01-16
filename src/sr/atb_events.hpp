#pragma once
#include <variant>

namespace seerin {

// InBus events (external)
struct Beat {}; // payload-free: one beat happened
struct AddCombatant {
  int id;
};

using AtbInEvent = std::variant<Beat, AddCombatant>;

// OutBus events (observable)
struct BecameReady {
  int id;
};

using AtbOutEvent = std::variant<BecameReady>;

// FSM-only event (internal fact)
struct BeatTick {}; // one ATB tick for a combatant FSM

} // namespace seerin
