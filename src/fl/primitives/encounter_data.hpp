#pragma once

#include <entt/entt.hpp>
#include <memory>
#include <vector>

#include "fl/events/battle_bus.hpp"
#include "fl/events/beat_bus.hpp"
#include "fl/events/ready_queue.hpp"
#include "fl/events/timed_event_queue.hpp"
#include "fl/primitives/team.hpp"
#include "fl/widgets/fancy_log.hpp"

#include "sr/atb_bus.hpp"
#include "sr/atb_engine.hpp"
#include "sr/encounter_bus.hpp"
#include "sr/encounter_events.hpp"
#include "sr/timed_scheduler.hpp"

namespace fl::primitives {

struct EncounterData {
public:
  EncounterData(fl::context::PartyCtx *party_ctx);

  EncounterData(EncounterData &&) noexcept = default;
  EncounterData &operator=(EncounterData &&) noexcept = default;

  EncounterData(const EncounterData &) = delete;
  EncounterData &operator=(const EncounterData &) = delete;

  // ---- accessors (refs out, pointers in) ----
  fl::primitives::Team &attackers() { return *attackers_; }
  const fl::primitives::Team &attackers() const { return *attackers_; }

  fl::primitives::Team &defenders() { return *defenders_; }
  const fl::primitives::Team &defenders() const { return *defenders_; }

  std::vector<entt::entity> &entities_to_cleanup() {
    return entities_to_cleanup_;
  }
  const std::vector<entt::entity> &entities_to_cleanup() const {
    return entities_to_cleanup_;
  }

  fl::events::BeatBus::Handle &beat_tick_handle() { return beat_tick_handle_; }
  const fl::events::BeatBus::Handle &beat_tick_handle() const {
    return beat_tick_handle_;
  }

  fl::events::BattleBus &battle_bus() { return battle_bus_; }
  const fl::events::BattleBus &battle_bus() const { return battle_bus_; }

  fl::events::TimedEventQueue &timed_events() { return timed_events_; }
  const fl::events::TimedEventQueue &timed_events() const {
    return timed_events_;
  }

  fl::events::ReadyQueue &ready_queue() { return ready_queue_; }
  const fl::events::ReadyQueue &ready_queue() const { return ready_queue_; }

  seerin::AtbInBus &atb_in() noexcept { return atb_in_; }
  const seerin::AtbInBus &atb_in() const noexcept { return atb_in_; }

  // ---- behavior ----
  bool has_alive_enemies();
  bool is_over();
  void finalize();
  void innervate_event_system(fl::events::BeatBus &beat_bus);

private:
  std::unique_ptr<fl::primitives::Team> attackers_;
  std::unique_ptr<fl::primitives::Team> defenders_;
  std::vector<entt::entity> entities_to_cleanup_{};
  fl::events::BeatBus::Handle beat_tick_handle_{};

  fl::events::BattleBus battle_bus_{};
  fl::events::TimedEventQueue timed_events_{};
  fl::events::ReadyQueue ready_queue_{};
  fl::context::PartyCtx *party_ctx_;

  seerin::AtbInBus atb_in_;
  seerin::AtbOutBus atb_out_;
  seerin::AtbEngine atb_;
  fl::events::PartyBus::Handle party_beat_handle_;

  fl::events::PartyBus::Handle party_tick_tap_;
};

struct InEncounter {
  /// @brief Backlink to the encounter entity that owns an Encounter component.
  entt::entity encounter_{entt::null};
};

void on_encounter_destroy(entt::registry &reg, entt::entity e);
void install_encounter_hooks(entt::registry &reg);

} // namespace fl::primitives
