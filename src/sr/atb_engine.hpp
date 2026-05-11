// atb_engine.hpp
#pragma once
#include <functional>
#include <unordered_map>
#include <vector>

#include <boost/sml.hpp>
#include <entt/entt.hpp>

#include "atb_events.hpp"
#include "atb_fsm.hpp"
#include "paired_bus.hpp"
#include "timed_scheduler.hpp"
#include "variant_bus.hpp"
#include "wire.hpp"

namespace seerin {

class AtbEngine {
public:
  using CanChargeFn = std::function<bool(entt::entity)>;

  AtbEngine();

  void set_can_charge_fn(CanChargeFn fn);
  void clear_pending_events();
  void clear_pending_events_for(entt::entity id);

  // ---- getters returning refs ----
  PairedBus<AtbInBus, AtbOutBus> &buses() { return buses_; }
  const AtbInBus &in() const noexcept { return buses_.in; }
  AtbInBus &in() { return buses_.in; }

  const AtbOutBus &out() const noexcept { return buses_.out; }
  AtbOutBus &out() { return buses_.out; }

  entt::entity &active_combatant() { return active_combatant_; }
  const entt::entity &active_combatant() const { return active_combatant_; }

  std::vector<entt::entity> &ready_queue() { return ready_queue_; }
  const std::vector<entt::entity> &ready_queue() const { return ready_queue_; }

  TimedScheduler<AtbOutEvent> &scheduler() { return scheduler_; }
  const TimedScheduler<AtbOutEvent> &scheduler() const { return scheduler_; }

private:
  // Internal “system bus” so we can use wire<> everywhere.
  using SysEvent = std::variant<Beat, AddCombatant, FinishedTurn, BecameReady>;
  using SysBus = VariantBus<SysEvent>;

  struct Combatant {
    AtbCtx ctx;
    boost::sml::sm<AtbMachine> sm;

    Combatant(entt::entity id, AtbOutBus &out_bus)
        : ctx{}, sm{AtbMachine{ctx, out_bus, id}} {}
  };

private:
  void on_add(const AddCombatant &e);
  void on_beat(const Beat &);
  void on_finished_turn(const FinishedTurn &e);

  // Response to out-events (via wire from out->sys_)
  void on_became_ready(const BecameReady &e);

  void enqueue_ready(entt::entity id);
  void pump_ready_queue(); // if no active and someone ready -> BecameActive
  bool can_charge(entt::entity id) const;
  void force_out_of_turn(entt::entity id);

private:
  PairedBus<AtbInBus, AtbOutBus> buses_;
  SysBus sys_;

  // Wiring handles (keep alive)
  decltype(wire<Beat>(buses_.in, sys_)) h_beat_wire_;
  decltype(wire<AddCombatant>(buses_.in, sys_)) h_add_wire_;
  decltype(wire<FinishedTurn>(buses_.in, sys_)) h_fin_wire_;
  decltype(wire<BecameReady>(buses_.out, sys_)) h_ready_wire_;

  // Subscriptions (keep alive)
  decltype(sys_.template on<Beat>([](const Beat &) {})) h_beat_sub_;
  decltype(sys_.template on<AddCombatant>(
      [](const AddCombatant &) {})) h_add_sub_;
  decltype(sys_.template on<FinishedTurn>(
      [](const FinishedTurn &) {})) h_fin_sub_;
  decltype(sys_.template on<BecameReady>(
      [](const BecameReady &) {})) h_ready_sub_;

  entt::entity active_combatant_{};
  std::vector<entt::entity> ready_queue_;

  std::unordered_map<entt::entity, Combatant> combatants_;
  TimedScheduler<AtbOutEvent> scheduler_;
  CanChargeFn can_charge_fn_ = [](entt::entity) { return true; };
};

} // namespace seerin
