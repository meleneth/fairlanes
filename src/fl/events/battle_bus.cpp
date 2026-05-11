#include "fl/events/battle_bus.hpp"

namespace fl::events {

void BattleBus::start_combat(entt::entity encounter) {
  emit(BattleEvent{StartCombat{encounter}});
}

void BattleBus::tick(std::chrono::milliseconds dt) {
  emit(BattleEvent{BattleTick{dt}});
}

void BattleBus::end_combat(entt::entity encounter, EndCombatReason reason) {
  emit(BattleEvent{EndCombat{encounter, reason}});
}

} // namespace fl::events
