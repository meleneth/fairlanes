#include <iostream>

#include "atb_engine.hpp"

#include <algorithm>

namespace seerin {

AtbEngine::AtbEngine() {
  // ---- Wire public buses into internal system bus ----
  h_beat_wire_ = wire<Beat>(buses_.in, sys_);
  h_add_wire_ = wire<AddCombatant>(buses_.in, sys_);
  h_fin_wire_ = wire<FinishedTurn>(buses_.in, sys_);

  // Wire engine outputs back into system bus so we can react to BecameReady
  h_ready_wire_ = wire<BecameReady>(buses_.out, sys_);

  // ---- Internal subscriptions ----
  h_add_sub_ =
      sys_.on<AddCombatant>([this](const AddCombatant &e) { on_add(e); });

  h_beat_sub_ = sys_.on<Beat>([this](const Beat &e) { on_beat(e); });

  h_fin_sub_ = sys_.on<FinishedTurn>(
      [this](const FinishedTurn &e) { on_finished_turn(e); });

  h_ready_sub_ = sys_.on<BecameReady>(
      [this](const BecameReady &e) { on_became_ready(e); });
}

void AtbEngine::on_add(const AddCombatant &e) {
  combatants_.try_emplace(e.id, e.id, buses_.out);
}

void AtbEngine::on_beat(const Beat &) {
  scheduler_.on_beat();

  // Tick all combatants
  for (auto &[id, c] : combatants_) {
    c.sm.process_event(BeatTick{});
  }

  // After ticking (which may enqueue via BecameReady),
  // attempt to activate someone.
  pump_ready_queue();
}

void AtbEngine::on_finished_turn(const FinishedTurn &e) {
  if (active_combatant_ == e.id) {
    active_combatant_ = entt::entity{};
    combatants_.at(e.id).sm.process_event(FinishedTurn{});
  }
}

void AtbEngine::on_became_ready(const BecameReady &e) { enqueue_ready(e.id); }

void AtbEngine::enqueue_ready(entt::entity id) { ready_queue_.push_back(id); }

void AtbEngine::pump_ready_queue() {
  if (ready_queue_.empty()) {
    return;
  }
  if (active_combatant_ != entt::entity{}) {
    return;
  }

  auto id = ready_queue_.front();
  ready_queue_.erase(ready_queue_.begin());
  active_combatant_ = id;

  buses_.out.emit(seerin::AtbOutEvent{BecameActive{id}});
}

} // namespace seerin
