#pragma once
#include <boost/sml.hpp>
#include <unordered_map>

#include <entt/entt.hpp>

#include "atb_fsm.hpp"

namespace seerin {

class AtbEngine {
public:
  AtbEngine(AtbInBus &in, AtbOutBus &out);

private:
  struct Combatant {
    AtbCtx ctx;
    boost::sml::sm<AtbMachine> sm;

    Combatant(entt::entity id, AtbOutBus &out_bus)
        : ctx{}, sm{AtbMachine{ctx, out_bus, id}} {}
  };

  void on_beat(const Beat &);
  void on_add(const AddCombatant &);

private:
  AtbInBus &in_;
  AtbOutBus &out_;
  std::unordered_map<entt::entity, Combatant> combatants_;
};

} // namespace seerin
