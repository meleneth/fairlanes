#include "fl/events/battle_bus.hpp"

namespace fl::events {

BattleEvent BattleEvent::make_start(entt::entity encounter) {
  BattleEvent ev{};
  ev.id = BattleEventId::StartCombat;
  ev.data = StartCombat{encounter};
  return ev;
}

BattleEvent BattleEvent::make_tick(std::chrono::milliseconds dt) {
  BattleEvent ev{};
  ev.id = BattleEventId::Tick;
  ev.data = Tick{dt};
  return ev;
}

BattleEvent BattleEvent::make_end(entt::entity encounter,
                                  EndCombatReason reason) {
  BattleEvent ev{};
  ev.id = BattleEventId::EndCombat;
  ev.data = EndCombat{encounter, reason};
  return ev;
}

BattleDispatcher::Handle BattleBus::add_listener(BattleEventId id,
                                                 Listener listener) {
  auto handle = dispatcher_.appendListener(id, std::move(listener));
  return handle;
}

void BattleBus::emit(const BattleEvent &ev) { dispatcher_.dispatch(ev.id, ev); }

void BattleBus::start_combat(entt::entity encounter) {
  emit(BattleEvent::make_start(encounter));
}

void BattleBus::tick(std::chrono::milliseconds dt) {
  emit(BattleEvent::make_tick(dt));
}

void BattleBus::end_combat(entt::entity encounter, EndCombatReason reason) {
  emit(BattleEvent::make_end(encounter, reason));
}

} // namespace fl::events
