#pragma once
#include <boost/sml.hpp>
#include <unordered_map>


#include "atb_fsm.hpp"

namespace seerin {

class AtbEngine {
public:
  AtbEngine(AtbInBus &in, AtbOutBus &out);

private:
  struct Combatant {
    AtbCtx ctx;
    boost::sml::sm<AtbMachine> sm;

    Combatant(int id, AtbOutBus &out_bus)
        : ctx{}, sm{AtbMachine{ctx, out_bus, id}} // <-- construct sm with deps
    {}
  };

  void on_beat(const Beat &);
  void on_add(const AddCombatant &);

private:
  AtbInBus &in_;
  AtbOutBus &out_;
  std::unordered_map<int, Combatant> combatants_;
};

} // namespace seerin
