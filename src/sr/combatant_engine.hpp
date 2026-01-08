#pragma once
// combatant_engine.hpp

#include "bus.hpp"
#include "combatant_fsm.hpp"
#include "overloaded.hpp"

#include <boost/sml.hpp>

namespace seerin::atb {

class CombatantEngine {
public:
  CombatantEngine(Config cfg, seerin::Bus<Event> &in,
                  seerin::Bus<OutputEvent> &out)
      : ctx_{.cfg = cfg}, sm_{ctx_, out},
        handle_(in.on([this](const Event &ev) {
          std::visit(
              seerin::overloaded{
                  [this](const Beat &e) { sm_.process_event(e); },
                  [this](const CommandSelected &e) { sm_.process_event(e); },
                  [this](const StunApplied &e) { sm_.process_event(e); },
                  [this](const Killed &e) { sm_.process_event(e); },
                  [this](const Revived &e) { sm_.process_event(e); }},
              ev);
        })) {}

  // test-friendly queries
  const Context &ctx() const { return ctx_; }

  bool is_charging() const {
    return sm_.is(boost::sml::state<CombatantFsmDef::Charging>);
  }
  bool is_ready() const {
    return sm_.is(boost::sml::state<CombatantFsmDef::Ready>);
  }
  bool is_acting() const {
    return sm_.is(boost::sml::state<CombatantFsmDef::Acting>);
  }

  bool is_dead() const {
    return sm_.is(boost::sml::state<CombatantFsmDef::Dead>);
  }

private:
  Context ctx_;
  boost::sml::sm<CombatantFsmDef> sm_;

  decltype(std::declval<seerin::Bus<Event>>().on([](auto const &) {})) handle_;
};

} // namespace seerin::atb
