#pragma once

#include <entt/entt.hpp>
#include <vector>

#include "fl/events/battle_bus.hpp"
#include "fl/events/beat_bus.hpp"
#include "fl/events/ready_queue.hpp"
#include "fl/events/timed_event_queue.hpp"
#include "fl/primitives/team.hpp"

namespace fl::ecs::components {

/**
 * @brief ECS component that models a single combat encounter.
 *
 * ### API surface
 * ```cpp
 * // Teams
 * std::unique_ptr<fl::primitives::Team> attackers_;
 * std::unique_ptr<fl::primitives::Team> defenders_;
 *
 * // Queries
 * bool has_alive_enemies();
 * bool is_over();
 *
 * // Lifecycle
 * void finalize();
 * void innervate_event_system();
 *
 * // Runtime systems
 * fl::events::BattleBus battle_bus_;
 * fl::events::TimedEventQueue timed_events_;
 * fl::events::ReadyQueue ready_queue_;
 * ```
 *
 * @details
 * An Encounter is typically attached to an "encounter entity" in the registry
 * and owns the encounter-scoped runtime systems:
 * - Teams (attackers and defenders)
 * - Event bus for battle-related events
 * - TimedEventQueue for animations and delayed effects
 * - ReadyQueue for ATB/initiative scheduling
 *
 * It also tracks temporary entities that should be cleaned up when the
 * encounter ends (e.g., spawned combatants, VFX marker entities, ephemeral UI
 * entities).
 *
 * @note
 * This component intentionally centralizes encounter-local wiring so the rest
 * of the game can treat an encounter as a single "arena" with its own
 * buses/queues.
 */
struct Encounter {
  /**
   * @brief Team representing the attacking side.
   *
   * @note Owned by the encounter. Typically built during encounter
   * initialization.
   */
  std::unique_ptr<fl::primitives::Team> attackers_{nullptr};

  /**
   * @brief Team representing the defending side.
   *
   * @note Owned by the encounter. Typically built during encounter
   * initialization.
   */
  std::unique_ptr<fl::primitives::Team> defenders_{nullptr};

  /**
   * @brief Entities to destroy or otherwise clean up when the encounter
   * finalizes.
   *
   * @details
   * This is used for encounter-scoped ephemeral entities that should not
   * survive after combat ends. If you add entities here, they are expected to
   * be valid in the registry at finalize time (or the finalize implementation
   * should tolerate already-destroyed entities).
   */

  std::vector<entt::entity> e_to_cleanup_;

  fl::events::BeatBus::Handle beat_tick_handle_;

  /**
   * @brief Returns true if the enemy side still has at least one living
   * combatant.
   *
   * @details
   * "Enemy side" is conventionally the defenders_ (or whichever side
   * represents the non-party team in your current encounter construction).
   *
   * @return true if at least one enemy is alive; otherwise false.
   */
  bool has_alive_enemies();

  /**
   * @brief Returns true if the encounter has reached a terminal state.
   *
   * @details
   * Typically this becomes true when one team is fully defeated, or when the
   * encounter is otherwise ended (flee, scripted exit, etc.).
   *
   * @return true if the encounter should stop processing; otherwise false.
   */
  bool is_over();

  /**
   * @brief Final cleanup for the encounter.
   *
   * @details
   * Expected responsibilities usually include:
   * - Destroying encounter-scoped ephemeral entities (e_to_cleanup_)
   * - Flushing or disabling encounter-local event processing
   * - Clearing team ownership if appropriate
   *
   * @warning
   * Be careful calling this while iterating registry views that include
   * encounter or cleanup entities. Prefer deferring cleanup or collecting
   * entities first.
   */
  void finalize();

  /**
   * @brief Wires the encounter's local event systems together.
   *
   * @details
   * This is the "plug the patch cables in" step. Common patterns include:
   * - Subscribing ReadyQueue output into BattleBus
   * - Translating BattleBus events into TimedEventQueue animations
   * - Installing any encounter-local handlers for combat resolution
   *
   * @note
   * This should be called after the encounter has teams/combatants created, so
   * handlers can reference stable entities and components.
   */
  void innervate_event_system(fl::events::BeatBus &beat_bus);

  /**
   * @brief Encounter-local event bus for combat events.
   *
   * @details
   * Used to publish and subscribe to battle-domain events without leaking them
   * onto account- or party-scoped buses.
   */
  fl::events::BattleBus battle_bus_;

  /**
   * @brief Queue for time-based or delayed events (e.g., animations).
   *
   * @details
   * Typically processed each tick with a delta time or absolute clock to
   * advance scheduled events.
   */
  fl::events::TimedEventQueue timed_events_;

  /**
   * @brief Queue for readiness/initiative scheduling (ATB-style or similar).
   *
   * @details
   * Holds combatants and the time at which they become ready to act.
   */
  fl::events::ReadyQueue ready_queue_;
};

/**
 * @brief Marker/backlink component indicating an entity is currently in an
 * Encounter.
 *
 * @details
 * This is usually attached to the party entity (or controlling entity) so
 * systems can quickly locate the current encounter without global lookups.
 *
 * The `encounter_` field points to the entity that owns the Encounter
 * component.
 */
struct InEncounter {
  /// @brief Backlink to the encounter entity that owns an Encounter component.
  entt::entity encounter_{entt::null};
};

/**
 * @brief Registry hook invoked when an encounter entity is destroyed.
 *
 * @details
 * Intended to keep the registry consistent when an encounter disappears:
 * - Remove/clear InEncounter on linked party entities
 * - Ensure finalize-like cleanup occurs (depending on how destruction happens)
 *
 * @warning
 * This runs during EnTT destruction context, so be mindful about modifying the
 * registry in ways that invalidate current iteration.
 */
void on_encounter_destroy(entt::registry &reg, entt::entity e);

/**
 * @brief Installs EnTT hooks related to Encounter lifecycle management.
 *
 * @details
 * Typical behavior includes connecting `on_encounter_destroy` to the registry
 * so encounter teardown logic runs automatically:
 * - reg.on_destroy<Encounter>().connect<&on_encounter_destroy>();
 *
 * Call this once during world/registry initialization.
 */
void install_encounter_hooks(entt::registry &reg);

} // namespace fl::ecs::components
