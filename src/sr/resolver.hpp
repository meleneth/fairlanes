#pragma once
// resolver.hpp

#include "encounter_events.hpp"

#include <vector>

namespace seerin::enc {

struct ResolveContext {
  // Keep framework-only stuff here.
  // Fairlanes can store identifiers to ECS, etc.
};

struct ICombatResolver {
  virtual ~ICombatResolver() = default;

  // Called when the active combatant finishes its timed action portion.
  // Return encounter-level framework events to apply (stun/kill/revive/etc).
  virtual std::vector<InEvent> resolve_action(CombatantId who,
                                              const ResolveContext &ctx) = 0;
};

} // namespace seerin::enc
