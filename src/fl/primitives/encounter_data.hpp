#pragma once
#include <algorithm>
#include <deque>
#include <entt/entt.hpp>
#include <memory>
#include <vector>

#include "fl/context.hpp"
#include "fl/events/battle_bus.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/events/ready_queue.hpp"
#include "fl/primitives/team.hpp"
#include "fl/skills/skill.hpp"
#include "sr/atb_bus.hpp"
#include "sr/atb_engine.hpp"

namespace fl::primitives {

struct EncounterData {
public:
  explicit EncounterData(fl::context::PartyCtx *party_ctx);

  EncounterData(EncounterData &&) = delete;
  EncounterData &operator=(EncounterData &&) = delete;

  EncounterData(const EncounterData &) = delete;
  EncounterData &operator=(const EncounterData &) = delete;

  // ---- accessors ----
  fl::primitives::Team &attackers() { return topo_.attackers_; }
  const fl::primitives::Team &attackers() const { return topo_.attackers_; }

  fl::primitives::Team &defenders() { return topo_.defenders_; }
  const fl::primitives::Team &defenders() const { return topo_.defenders_; }

  std::vector<entt::entity> &entities_to_cleanup() {
    return life_.entities_to_cleanup_;
  }

  const std::vector<entt::entity> &entities_to_cleanup() const {
    return life_.entities_to_cleanup_;
  }

  fl::events::BattleBus &battle_bus() { return rt_.battle_bus_; }
  const fl::events::BattleBus &battle_bus() const { return rt_.battle_bus_; }

  fl::events::EncounterBus &encounter_bus() { return rt_.encounter_bus_; }
  const fl::events::EncounterBus &encounter_bus() const {
    return rt_.encounter_bus_;
  }

  struct EnemyCombatantBus {
    entt::entity enemy{entt::null};
    fl::events::CombatantBus bus;
  };

  std::deque<EnemyCombatantBus> &enemy_combatant_buses() {
    return rt_.enemy_combatant_buses_;
  }
  const std::deque<EnemyCombatantBus> &enemy_combatant_buses() const {
    return rt_.enemy_combatant_buses_;
  }

  fl::events::CombatantBus &add_enemy_combatant_bus(entt::entity enemy);
  fl::events::CombatantBus &add_party_combatant_bus(entt::entity member);
  fl::events::CombatantBus &combatant_bus(entt::entity combatant);
  const fl::events::CombatantBus &combatant_bus(entt::entity combatant) const;
  fl::events::CombatantBus *enemy_combatant_bus(entt::entity enemy);
  const fl::events::CombatantBus *enemy_combatant_bus(entt::entity enemy) const;

  fl::events::ReadyQueue &ready_queue() { return rt_.ready_queue_; }
  const fl::events::ReadyQueue &ready_queue() const { return rt_.ready_queue_; }

  seerin::AtbEngine &atb_engine() noexcept { return rt_.atb_; }
  const seerin::AtbEngine &atb_engine() const noexcept { return rt_.atb_; }

  seerin::AtbInBus &atb_in() noexcept { return rt_.atb_.in(); }
  const seerin::AtbInBus &atb_in() const noexcept { return rt_.atb_.in(); }

  seerin::AtbOutBus &atb_out() noexcept { return rt_.atb_.out(); }
  const seerin::AtbOutBus &atb_out() const noexcept { return rt_.atb_.out(); }

  // ---- behavior ----
  bool has_alive_enemies();
  bool is_over();
  void finalize();
  void clear_pending_events();
  void clear_active_turn_for(entt::entity id);
  void innervate_event_system();
  std::size_t pending_scheduled_events() const {
    return rt_.atb_.scheduler().pending();
  }

  seerin::uWu visual_time() const { return rt_.atb_.scheduler().now(); }

  bool owns_entity(entt::entity e) const {
    return std::find(life_.entities_to_cleanup_.begin(),
                     life_.entities_to_cleanup_.end(),
                     e) != life_.entities_to_cleanup_.end();
  }

  bool is_good_guy(entt::entity e) const { return !owns_entity(e); }
  bool is_bad_guy(entt::entity e) const { return owns_entity(e); }

  entt::entity target_random_alive_opposition(entt::entity e) const {
    if (topo_.attackers_.contains(e)) {
      return topo_.defenders_.random_alive_member(*party_ctx_)
          .value_or(entt::null);
    }

    if (topo_.defenders_.contains(e)) {
      return topo_.attackers_.random_alive_member(*party_ctx_)
          .value_or(entt::null);
    }

    return entt::null;
  }

  entt::entity target_for_skill(entt::entity attacker,
                                fl::skills::SkillKey skill) const;
  fl::skills::SkillKey choose_skill(entt::entity attacker);

private:
  struct Topology {
    Team attackers_;
    Team defenders_;
  } topo_;

  struct Runtime {
    seerin::AtbEngine atb_;
    fl::events::ReadyQueue ready_queue_;
    fl::events::BattleBus battle_bus_;
    fl::events::EncounterBus encounter_bus_;
    std::deque<EnemyCombatantBus> enemy_combatant_buses_;
  } rt_;

  struct Wiring {
    struct CombatantWiring {
      fl::events::ScopedCombatantListener poison_apply_;
      fl::events::ScopedCombatantListener freeze_apply_;
      fl::events::ScopedCombatantListener freeze_started_;
      fl::events::ScopedCombatantListener freeze_ended_;
    };

    fl::events::ScopedPartyListener party_beat_;
    std::deque<CombatantWiring> combatant_wiring_;
    std::vector<entt::entity> wired_combatants_;
    seerin::BecameActiveSub atb_active_;
  } wire_;

  struct Lifecycle {
    std::vector<entt::entity> entities_to_cleanup_;
  } life_;

  void bind_combatant_bus(entt::entity combatant,
                          fl::events::CombatantBus &combatant_bus);

  fl::context::PartyCtx *party_ctx_;
};

} // namespace fl::primitives
