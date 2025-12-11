#pragma once
#include <entt/entt.hpp>
#include <vector>

namespace fl::ecs::components {

struct Encounter {

  std::unique_ptr<fl::concepts::Team> attackers_ = nullptr;
  std::unique_ptr<fl::concepts::Team> defenders_ = nullptr;
  std::vector<entt::entity> e_to_cleanup_;
  bool has_alive_enemies();
  bool is_over();
  void finalize();
};

struct InEncounter {                   // attach to the party
  entt::entity encounter_{entt::null}; // backlink to the encounter entity
};

void on_encounter_destroy(entt::registry &reg, entt::entity e);
void install_encounter_hooks(entt::registry &reg);
} // namespace fl::ecs::components
