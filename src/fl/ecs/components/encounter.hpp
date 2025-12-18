#pragma once
#include <entt/entt.hpp>
#include <vector>

#include "fl/events/battle_bus.hpp"
#include "fl/events/ready_queue.hpp"
#include "fl/events/timed_event_queue.hpp"
#include "fl/primitives/team.hpp"


namespace fl::ecs::components {

struct Encounter {

  std::unique_ptr<fl::primitives::Team> attackers_{nullptr};
  std::unique_ptr<fl::primitives::Team> defenders_{nullptr};
  std::vector<entt::entity> e_to_cleanup_;
  bool has_alive_enemies();
  bool is_over();
  void finalize();
  void innervate_event_system();
  fl::events::BattleBus battle_bus_;
  fl::events::TimedEventQueue timed_events_;
  fl::events::ReadyQueue ready_queue_;
};

struct InEncounter {                   // attach to the party
  entt::entity encounter_{entt::null}; // backlink to the encounter entity
};

void on_encounter_destroy(entt::registry &reg, entt::entity e);
void install_encounter_hooks(entt::registry &reg);
} // namespace fl::ecs::components
