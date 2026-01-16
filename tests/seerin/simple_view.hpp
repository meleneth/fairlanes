#pragma once
#include "sr/encounter_view.hpp"
#include <unordered_map>

namespace seerin::test {

struct SimpleView {
  std::unordered_map<CombatantId, bool> alive;
  std::unordered_map<CombatantId, TeamId> team;

  bool is_alive(CombatantId id) const {
    auto it = alive.find(id);
    return it != alive.end() && it->second;
  }

  TeamId team_of(CombatantId id) const {
    auto it = team.find(id);
    return it != team.end() ? it->second : TeamId{-1};
  }
};

static_assert(seerin::EncounterView<SimpleView>);

} // namespace seerin::test
