#include "atb_engine.hpp"

namespace seerin {

AtbEngine::AtbEngine(AtbInBus &in, AtbOutBus &out) : in_{in}, out_{out} {
  in_.on<Beat>([this](auto const &e) { on_beat(e); });
  in_.on<AddCombatant>([this](auto const &e) { on_add(e); });
}

void AtbEngine::on_add(const AddCombatant &e) {
  // emplace combatant if missing
  combatants_.try_emplace(e.id, e.id, out_);
}

void AtbEngine::on_beat(const Beat &) {
  for (auto &[id, c] : combatants_) {
    c.sm.process_event(BeatTick{});
  }
}

} // namespace seerin
