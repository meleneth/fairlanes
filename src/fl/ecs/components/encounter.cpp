#include "encounter.hpp"
#include "fl/ecs/components/is_party.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/primitives/team.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::components {

void on_encounter_destroy(entt::registry &reg, entt::entity e) {
  auto &enc = reg.get<Encounter>(e); // valid: signal fires before removal
  enc.finalize();
}

void install_encounter_hooks(entt::registry &reg) {
  reg.on_destroy<Encounter>().connect<&on_encounter_destroy>();
}

void Encounter::finalize() {
  /* for (auto e_cleanup : e_to_cleanup_) {
      ctx_.reg_.destroy(e_cleanup);
    }
  */
  // ctx_.log_.append_markup(fmt::format("Encounter {} finalized and cleaned
  // up",
  //                                     int(entt::to_integral(ctx_.self_))));
}

bool Encounter::has_alive_enemies() {
  using fl::ecs::components::Stats;
  /*  for (auto e : e_to_cleanup_) {
      if (!ctx_.reg_.valid(e) || !ctx_.reg_.all_of<Stats>(e)) {
        continue; // stale or already destroyed, ignore
      }

      auto &enemy = ctx_.reg_.get<Stats>(e);
      if (enemy.is_alive()) {
        return true;
      }
    }
  */
  return false;
}

bool Encounter::is_over() {
  // For now, "over" simply means there are no alive enemies.
  return !has_alive_enemies();
}
} // namespace fl::ecs::components
