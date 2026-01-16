#pragma once
#include "encounter_types.hpp"
#include <concepts>

namespace seerin {

template <class T>
concept EncounterView = requires(const T &v, CombatantId id) {
  { v.is_alive(id) } -> std::convertible_to<bool>;
  { v.team_of(id) } -> std::convertible_to<TeamId>;
};

} // namespace seerin
