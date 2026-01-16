#pragma once
#include "atb_events.hpp"
#include "encounter_types.hpp"

#include <variant>

namespace seerin {

// generic action intent (skills are data)
using SkillId = int32_t;
struct ActionRequested {
  CombatantId caster;
  SkillId skill;
  CombatantId target;
};

// generic effect (MVP placeholder; expand later)
struct ApplyEffect {
  CombatantId src;
  CombatantId dst;
  SkillId skill;
  int32_t magnitude;
};

using EncounterEvent =
    std::variant<Beat, AddCombatant, BecameReady, ActionRequested, ApplyEffect>;

} // namespace seerin
