#pragma once
#include <algorithm>
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

  fl::events::ReadyQueue &ready_queue() { return rt_.ready_queue_; }
  const fl::events::ReadyQueue &ready_queue() const { return rt_.ready_queue_; }

  seerin::AtbInBus &atb_in() noexcept { return rt_.atb_.in(); }
  const seerin::AtbInBus &atb_in() const noexcept { return rt_.atb_.in(); }

  seerin::AtbOutBus &atb_out() noexcept { return rt_.atb_.out(); }
  const seerin::AtbOutBus &atb_out() const noexcept { return rt_.atb_.out(); }

  // ---- behavior ----
  bool has_alive_enemies();
  bool is_over();
  void finalize();
  void clear_pending_events();
  void clear_pending_events_for(entt::entity id);
  void innervate_event_system();
  std::size_t pending_scheduled_events() const {
    return rt_.atb_.scheduler().pending();
  }

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

  fl::skills::SkillId choose_skill(entt::entity attacker);

private:
  struct Topology {
    Team attackers_;
    Team defenders_;
  } topo_;

  struct Runtime {
    seerin::AtbEngine atb_;
    fl::events::ReadyQueue ready_queue_;
    fl::events::BattleBus battle_bus_;
  } rt_;

  struct Wiring {
    fl::events::ScopedPartyListener party_beat_;
    seerin::BecameActiveSub atb_active_;
  } wire_;

  struct Lifecycle {
    std::vector<entt::entity> entities_to_cleanup_;
  } life_;

  fl::context::PartyCtx *party_ctx_;
};

} // namespace fl::primitives
