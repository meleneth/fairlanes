#include "atb_engine.hpp"

#include <algorithm>
#include <tracy/Tracy.hpp>
#include <utility>

#include "fl/ecs/components/atb_charge.hpp"

namespace seerin {

AtbEngine::AtbEngine() {
  // ---- Wire public buses into internal system bus ----
  h_beat_wire_ = wire<Beat>(buses_.in, sys_);
  h_add_wire_ = wire<AddCombatant>(buses_.in, sys_);
  h_fin_wire_ = wire<FinishedTurn>(buses_.in, sys_);

  // Wire engine outputs back into system bus so we can react to BecameReady
  h_ready_wire_ = wire<BecameReady>(buses_.out, sys_);

  // ---- Internal subscriptions ----
  h_add_sub_ = sys_.subscribe<AddCombatant>(
      [this](const AddCombatant &e) { on_add(e); });

  h_beat_sub_ = sys_.subscribe<Beat>([this](const Beat &e) { on_beat(e); });

  h_fin_sub_ = sys_.subscribe<FinishedTurn>(
      [this](const FinishedTurn &e) { on_finished_turn(e); });

  h_ready_sub_ = sys_.subscribe<BecameReady>(
      [this](const BecameReady &e) { on_became_ready(e); });
}

AtbEngine::AtbEngine(entt::registry &reg) : AtbEngine() { bind_registry(reg); }

void AtbEngine::set_can_charge_fn(CanChargeFn fn) {
  if (fn) {
    can_charge_fn_ = std::move(fn);
    return;
  }

  can_charge_fn_ = [](entt::entity) { return true; };
}

void AtbEngine::clear_pending_events() {
  ZoneScopedN("AtbEngine::clear_pending_events");
  scheduler_.clear();
  ready_queue_.clear();
  active_combatant_ = entt::entity{};
}

void AtbEngine::clear_pending_events_for(entt::entity id) {
  ZoneScopedN("AtbEngine::clear_pending_events_for");
  // Remove from ready queue
  ready_queue_.erase(std::remove(ready_queue_.begin(), ready_queue_.end(), id),
                     ready_queue_.end());

  // Reset active combatant if it's this entity
  if (active_combatant_ == id) {
    active_combatant_ = entt::entity{};
  }

  // Remove scheduled events for this entity
  scheduler_.remove_if([id](const TimedScheduler<AtbOutEvent>::TimedAction
                                &action) {
    if (action.valueless_by_exception()) {
      return false;
    }

    return std::visit(
        [id](auto &&ev) -> bool {
          using T = std::decay_t<decltype(ev)>;
          if constexpr (std::is_same_v<
                            T, TimedScheduler<AtbOutEvent>::EmitEvent>) {
            if (ev.ev.valueless_by_exception()) {
              return false;
            }

            return std::visit(
                [id](auto &&out_ev) -> bool {
                  using OutT = std::decay_t<decltype(out_ev)>;
                  if constexpr (std::is_same_v<OutT, BecameReady>) {
                    return out_ev.id == id;
                  } else if constexpr (std::is_same_v<OutT, BecameActive>) {
                    return out_ev.id == id;
                  }
                  return false;
                },
                ev.ev);
          } else if constexpr (std::is_same_v<T, TimedScheduler<AtbOutEvent>::
                                                     SmellyCallback>) {
            return ev.owner == id;
          }
          return false;
        },
        action);
  });
}

void AtbEngine::on_add(const AddCombatant &e) {
  ZoneScopedN("AtbEngine::on_add");
  if (reg_ == nullptr || !reg_->valid(e.id)) {
    return;
  }

  reg_->emplace_or_replace<fl::ecs::components::AtbCharge>(e.id);
  combatants_.try_emplace(e.id, *reg_, e.id, buses_.out);
  TracyPlot("ATB.Combatants", static_cast<double>(combatants_.size()));
}

void AtbEngine::on_beat(const Beat &) {
  ZoneScopedN("AtbEngine::on_beat");
  TracyPlot("ATB.ReadyQueue", static_cast<double>(ready_queue_.size()));
  TracyPlot("ATB.Combatants", static_cast<double>(combatants_.size()));

  scheduler_.on_beat();

  if (reg_ == nullptr) {
    return;
  }

  // Tick all combatants that are allowed to charge.
  std::vector<entt::entity> remove_ids;
  for (auto &[id, c] : combatants_) {
    if (!reg_->valid(id) || !reg_->all_of<fl::ecs::components::AtbCharge>(id)) {
      force_out_of_turn(id);
      remove_ids.push_back(id);
      continue;
    }

    if (!can_charge(id)) {
      reg_->get<fl::ecs::components::AtbCharge>(id).charge = 0;
      c.sm.process_event(FinishedTurn{});
      force_out_of_turn(id);
      continue;
    }

    c.sm.process_event(BeatTick{});
  }

  for (auto id : remove_ids) {
    combatants_.erase(id);
  }

  // After ticking (which may enqueue via BecameReady),
  // attempt to activate someone.
  pump_ready_queue();
}

void AtbEngine::on_finished_turn(const FinishedTurn &e) {
  ZoneScopedN("AtbEngine::on_finished_turn");
  if (active_combatant_ == e.id) {
    active_combatant_ = entt::entity{};
    auto it = combatants_.find(e.id);
    if (it != combatants_.end()) {
      it->second.sm.process_event(FinishedTurn{});
    }
  }
}

void AtbEngine::on_became_ready(const BecameReady &e) { enqueue_ready(e.id); }

void AtbEngine::enqueue_ready(entt::entity id) { ready_queue_.push_back(id); }

void AtbEngine::pump_ready_queue() {
  ZoneScopedN("AtbEngine::pump_ready_queue");
  if (active_combatant_ != entt::entity{}) {
    return;
  }

  while (!ready_queue_.empty()) {
    auto id = ready_queue_.front();
    ready_queue_.erase(ready_queue_.begin());

    if (!can_charge(id)) {
      continue;
    }

    active_combatant_ = id;
    buses_.out.emit(seerin::AtbOutEvent{BecameActive{id}});
    TracyPlot("ATB.ReadyQueue", static_cast<double>(ready_queue_.size()));
    return;
  }
}

bool AtbEngine::can_charge(entt::entity id) const { return can_charge_fn_(id); }

void AtbEngine::force_out_of_turn(entt::entity id) {
  ready_queue_.erase(std::remove(ready_queue_.begin(), ready_queue_.end(), id),
                     ready_queue_.end());

  if (active_combatant_ == id) {
    active_combatant_ = entt::entity{};
  }
}

} // namespace seerin
