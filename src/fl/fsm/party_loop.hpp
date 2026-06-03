#pragma once
#include <boost/sml.hpp>

#include "fl/context.hpp"

namespace fl::fsm {
namespace sml = boost::sml;

struct NextEvent {};
struct PartyWipedEvent {};

struct PartyLoop {
  // Put all behavior in a ctx-only “ops” namespace/struct.
  struct Ops {
    static void enter_idle(fl::context::PartyCtx &ctx);
    static void enter_farming(fl::context::PartyCtx &ctx);
    static void exit_farming(fl::context::PartyCtx &ctx);
    static void enter_dead(fl::context::PartyCtx &ctx);
    static void enter_fixing(fl::context::PartyCtx &ctx);
    static void exit_fixing(fl::context::PartyCtx &ctx);
    static void enter_gearing(fl::context::PartyCtx &ctx);
    static void fixing_tick(fl::context::PartyCtx &ctx);

    static void combat_tick(fl::context::PartyCtx &ctx);
    static void next_event(fl::context::PartyCtx &ctx);

    static bool in_combat(fl::context::PartyCtx &ctx);
    static bool fixing_done(fl::context::PartyCtx &ctx);
  };

  struct EnterIdle {
    void operator()(fl::context::PartyCtx &ctx) const { Ops::enter_idle(ctx); }
  };
  struct EnterFarming {
    void operator()(fl::context::PartyCtx &ctx) const {
      Ops::enter_farming(ctx);
    }
  };
  struct ExitFarming {
    void operator()(fl::context::PartyCtx &ctx) const {
      Ops::exit_farming(ctx);
    }
  };
  struct EnterDead {
    void operator()(fl::context::PartyCtx &ctx) const { Ops::enter_dead(ctx); }
  };
  struct EnterFixing {
    void operator()(fl::context::PartyCtx &ctx) const {
      Ops::enter_fixing(ctx);
    }
  };
  struct ExitFixing {
    void operator()(fl::context::PartyCtx &ctx) const { Ops::exit_fixing(ctx); }
  };
  struct EnterGearing {
    void operator()(fl::context::PartyCtx &ctx) const {
      Ops::enter_gearing(ctx);
    }
  };
  struct CombatTick {
    void operator()(fl::context::PartyCtx &ctx) const { Ops::combat_tick(ctx); }
  };
  struct FixingTick {
    void operator()(fl::context::PartyCtx &ctx) const { Ops::fixing_tick(ctx); }
  };
  struct InCombat {
    bool operator()(fl::context::PartyCtx &ctx) const {
      return Ops::in_combat(ctx);
    }
  };
  struct FixingDone {
    bool operator()(fl::context::PartyCtx &ctx) const {
      return Ops::fixing_done(ctx);
    }
  };
  auto operator()() const {
    using namespace sml;

    struct Idle {};
    struct Farming {};
    struct Dead {};
    struct Fixing {};
    struct Gearing {};
    struct CombatIdle {};

    return make_transition_table(
        *state<Idle> + sml::on_entry<_> / EnterIdle{},
        state<Farming> + sml::on_entry<_> / EnterFarming{},
        state<Dead> + sml::on_entry<_> / EnterDead{},
        state<Fixing> + sml::on_entry<_> / EnterFixing{},
        state<Gearing> + sml::on_entry<_> / EnterGearing{},

        state<Idle> + event<NextEvent> = state<Farming>,
        state<Idle> + event<PartyWipedEvent> = state<Dead>,
        state<Farming> + event<PartyWipedEvent> = state<Dead>,
        state<CombatIdle> + event<PartyWipedEvent> = state<Dead>,
        state<Fixing> + event<PartyWipedEvent> = state<Dead>,
        state<Gearing> + event<PartyWipedEvent> = state<Dead>,
        state<Dead> + event<PartyWipedEvent> = state<Dead>,
        state<Farming> + event<NextEvent>[InCombat{}] / CombatTick{},
        state<Farming> + event<NextEvent> / ExitFarming{} = state<CombatIdle>,
        state<CombatIdle> + event<NextEvent> = state<Gearing>,
        state<Dead> + event<NextEvent> = state<Fixing>,
        state<Fixing> + event<NextEvent>[FixingDone{}] / ExitFixing{} =
            state<Gearing>,
        state<Fixing> + event<NextEvent> / FixingTick{} = state<Fixing>,
        state<Gearing> + event<NextEvent> = state<Idle>);
  }
};

} // namespace fl::fsm
