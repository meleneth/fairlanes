#pragma once
// encounter_loop.hpp

#include "encounter_events.hpp"
#include "ready_queue.hpp"
#include "resolver.hpp"

#include "bus.hpp"
#include "overloaded.hpp"

// ATB engine
#include "atb_events.hpp"
#include "combatant_engine.hpp"


#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace seerin::enc {

class EncounterLoop {
public:
  EncounterLoop(seerin::Bus<InEvent> &in, seerin::Bus<OutEvent> &out,
                ICombatResolver &resolver, std::vector<CombatantId> combatants,
                seerin::atb::Config atb_cfg);

  // test-friendly queries
  std::optional<CombatantId> active() const { return active_; }
  std::optional<CombatantId> awaiting_command() const {
    return awaiting_command_;
  }
  std::vector<CombatantId> ready_queue_snapshot() const {
    return ready_.snapshot();
  }

private:
  // external input handling
  void on_in(const InEvent &ev);

  void handle_beat(const Beat &b);
  void handle_command_chosen(const CommandChosen &c);
  void handle_apply_stun(const ApplyStun &s);
  void handle_kill(const Kill &k);
  void handle_revive(const Revive &r);

  // engine output handling
  void on_combatant_out(CombatantId who, const seerin::atb::OutputEvent &ev);

  // orchestration helpers
  void enqueue_ready(CombatantId who);
  void maybe_request_command();
  void emit_ready_queue_changed();
  void set_active(std::optional<CombatantId> who);

  void forward_to_combatant(CombatantId who, const seerin::atb::Event &ev);

  struct Slot {
    seerin::Bus<seerin::atb::Event> in{};
    seerin::Bus<seerin::atb::OutputEvent> out{};
    seerin::atb::CombatantEngine engine;

    Slot(seerin::atb::Config cfg) : in{}, out{}, engine(cfg, in, out) {}
  };

  seerin::Bus<OutEvent> &out_;
  ICombatResolver &resolver_;

  // stable order for determinism
  std::vector<CombatantId> combatants_{};

  // per combatant engines
  std::unordered_map<CombatantId, std::unique_ptr<Slot>> slots_{};

  ReadyQueue ready_{};

  // single-active pipeline
  std::optional<CombatantId> active_{};
  std::optional<CombatantId> awaiting_command_{};

  // subscriptions
  decltype(std::declval<seerin::Bus<InEvent>>().on(
      [](auto const &) {})) in_handle_;
  std::vector<decltype(std::declval<seerin::Bus<seerin::atb::OutputEvent>>().on(
      [](auto const &) {}))>
      slot_out_handles_;
};

} // namespace seerin::enc
