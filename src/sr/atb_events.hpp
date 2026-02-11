#pragma once
#include <variant>

#include <entt/entt.hpp>

namespace seerin {

// InBus events (external)
struct Beat {}; // payload-free: one beat happened
struct AddCombatant {
  entt::entity id;
};

using AtbInEvent = std::variant<Beat, AddCombatant>;

// OutBus events (observable)
struct BecameReady {
  entt::entity id;
};

using AtbOutEvent = std::variant<BecameReady>;

// FSM-only event (internal fact)
struct BeatTick {}; // one ATB tick for a combatant FSM

} // namespace seerin
