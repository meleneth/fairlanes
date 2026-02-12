// atb_events.hpp
#pragma once
#include <entt/entt.hpp>
#include <variant>

namespace seerin {

// InBus events (external)
struct Beat {};
struct AddCombatant {
  entt::entity id;
};

// “Turn spent; start charging again”
struct FinishedTurn {
  entt::entity id;
};

using AtbInEvent = std::variant<Beat, AddCombatant, FinishedTurn>;

// OutBus events (observable)
struct BecameReady {
  entt::entity id;
};

// NEW: ATB chose who is active now
struct BecameActive {
  entt::entity id;
};

using AtbOutEvent = std::variant<BecameReady, BecameActive>;

// FSM-only events (internal)
struct BeatTick {};

} // namespace seerin
