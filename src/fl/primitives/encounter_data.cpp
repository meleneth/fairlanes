#include "encounter_data.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/events/beat_bus.hpp"
#include "fl/primitives/encounter_data.hpp"

namespace fl::primitives {

void on_encounter_destroy(entt::registry &reg, entt::entity e) {
  auto &enc = reg.get<fl::ecs::components::Encounter>(
      e); // valid: signal fires before removal
  enc.encounter_data().finalize();
}

void install_encounter_hooks(entt::registry &reg) {

  reg.on_destroy<fl::ecs::components::Encounter>()
      .connect<&on_encounter_destroy>();
}

void EncounterData::innervate_event_system(fl::events::BeatBus &beat_bus) {
  // Upstream: global heartbeat drives battle bus.
  // Incorrect. PARTY subscribes to heart beat
  /*
    beat_tick_handle_ = beat_bus.add_listener(
        fl::events::BeatEventId::Beat, [this](const fl::events::BeatEvent &ev) {
          const auto &beat = std::get<fl::events::BeatPulse>(ev.data).beat;

          battle_bus_.tick(beat.dt);
          timed_events_.advance(beat.dt);
        });
        */
  (void)beat_bus;
}

void EncounterData::finalize() {
  //  ctx_.log().append_markup(
  //    fmt::format("Finalizing encounter with {} entities to clean up",
  //              entities_to_cleanup_.size()));
  /* for (auto e_cleanup : e_to_cleanup_) {
      ctx_.reg_.destroy(e_cleanup);
    }
  */
  // ctx_.log_.append_markup(fmt::format("Encounter {} finalized and cleaned
  // up",
  //                                     int(entt::to_integral(ctx_.self_))));
}

bool EncounterData::has_alive_enemies() {
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

bool EncounterData::is_over() {
  // For now, "over" simply means there are no alive enemies.
  return !has_alive_enemies();
}
} // namespace fl::primitives
