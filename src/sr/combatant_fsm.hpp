#pragma once
// combatant_fsm.hpp

#include "atb_context.hpp"
#include "bus.hpp"
#include "overloaded.hpp"

#include <boost/sml.hpp>

namespace seerin::atb {

namespace sml = boost::sml;

struct CombatantFsmDef {
  // State types
  struct Charging {};
  struct Ready {};
  struct Acting {};

  struct ChargingStunned {};
  struct ReadyStunned {};
  struct ActingStunned {};

  struct Dead {};

  auto operator()() const {
    using namespace sml;

    // Dependencies injected into sm: Context&, Bus<OutputEvent>&
    const auto emit_state = [](StateTag from, StateTag to) {
      return [from, to](Bus<OutputEvent> &out) {
        out.emit(OutputEvent{StateChanged{from, to}});
      };
    };

    const auto emit_ready = [](Bus<OutputEvent> &out) {
      out.emit(OutputEvent{BecameReady{}});
    };
    /*
    const auto emit_started = [](const CommandSelected &c,
                                 Bus<OutputEvent> &out) {
      out.emit(OutputEvent{ActionStarted{c.action_time}});
    };
    const auto emit_finished = [](Bus<OutputEvent> &out) {
      out.emit(OutputEvent{ActionFinished{}});
    };
*/
    // Guards
    const auto charge_will_fill = [](const Beat &b, const Context &ctx) {
      return ctx.will_fill_on(b);
    };
    const auto action_will_finish = [](const Beat &b, const Context &ctx) {
      return ctx.will_finish_action_on(b);
    };
    const auto stun_will_end = [](const Beat &b, const Context &ctx) {
      return ctx.will_end_stun_on(b);
    };
    const auto is_instant = [](const CommandSelected &c) {
      return c.action_time.count() <= 0;
    };
    const auto revive_full = [](const Revived &r, const Context &ctx) {
      return Context::clamp_i64(r.initial_charge_units, 0,
                                ctx.cfg.max_charge_units) >=
             ctx.cfg.max_charge_units;
    };

    // Actions
    const auto charge_tick = [](const Beat &b, Context &ctx) {
      ctx.apply_charge(b);
    };

    const auto charge_to_full = [](const Beat &b, Context &ctx,
                                   Bus<OutputEvent> &out) {
      ctx.apply_charge(b);
      ctx.charge_units = ctx.cfg.max_charge_units;
      out.emit(OutputEvent{BecameReady{}});
    };

    const auto start_action = [](const CommandSelected &c, Context &ctx,
                                 Bus<OutputEvent> &out) {
      ctx.action_remaining = c.action_time;
      out.emit(OutputEvent{ActionStarted{c.action_time}});
    };

    const auto instant_action = [](const CommandSelected &, Context &ctx,
                                   Bus<OutputEvent> &out) {
      out.emit(OutputEvent{ActionStarted{ns{0}}});
      out.emit(OutputEvent{ActionFinished{}});
      ctx.reset_after_action();
    };

    const auto action_tick = [](const Beat &b, Context &ctx) {
      ctx.tick_action(b);
    };

    const auto finish_action = [](const Beat &b, Context &ctx,
                                  Bus<OutputEvent> &out) {
      ctx.tick_action(b); // consumes remaining
      out.emit(OutputEvent{ActionFinished{}});
      ctx.reset_after_action();
    };

    const auto apply_stun = [](const StunApplied &s, Context &ctx) {
      ctx.stun_remaining = std::max(ctx.stun_remaining, s.duration);
    };
    const auto stun_tick = [](const Beat &b, Context &ctx) {
      ctx.tick_stun(b);
    };

    const auto kill = [](Context &ctx) {
      ctx.charge_units = 0;
      ctx.accum = 0;
      ctx.action_remaining = ns{0};
      ctx.stun_remaining = ns{0};
    };

    const auto revive = [](const Revived &r, Context &ctx) {
      ctx.charge_units = Context::clamp_i64(r.initial_charge_units, 0,
                                            ctx.cfg.max_charge_units);
      ctx.accum = 0;
      ctx.action_remaining = ns{0};
      ctx.stun_remaining = ns{0};
    };

    // Table
    return make_transition_table(
        // clang-format off

        // Charging: Beat either fills -> Ready, else stay Charging
        *state<Charging> + event<Beat>[charge_will_fill] / (charge_to_full, emit_state(StateTag::Charging, StateTag::Ready)) = state<Ready>,
        state<Charging>  + event<Beat>                   / charge_tick = state<Charging>,

        // Ready: command chosen -> Acting or instant -> Charging
        state<Ready> + event<CommandSelected>[is_instant] / (instant_action, emit_state(StateTag::Ready, StateTag::Charging)) = state<Charging>,
        state<Ready> + event<CommandSelected>             / (start_action, emit_state(StateTag::Ready, StateTag::Acting))     = state<Acting>,

        // Acting: Beat either finishes -> Charging, else keep Acting
        state<Acting> + event<Beat>[action_will_finish] / (finish_action, emit_state(StateTag::Acting, StateTag::Charging)) = state<Charging>,
        state<Acting> + event<Beat>                     / action_tick = state<Acting>,

        // Stun transitions (mirror states)
        state<Charging> + event<StunApplied> / (apply_stun, emit_state(StateTag::Charging, StateTag::ChargingStunned)) = state<ChargingStunned>,
        state<Ready>    + event<StunApplied> / (apply_stun, emit_state(StateTag::Ready,    StateTag::ReadyStunned))    = state<ReadyStunned>,
        state<Acting>   + event<StunApplied> / (apply_stun, emit_state(StateTag::Acting,   StateTag::ActingStunned))   = state<ActingStunned>,

        state<ChargingStunned> + event<Beat>[stun_will_end] / (stun_tick, emit_state(StateTag::ChargingStunned, StateTag::Charging)) = state<Charging>,
        state<ChargingStunned> + event<Beat>                / stun_tick                                                                       = state<ChargingStunned>,

        state<ReadyStunned> + event<Beat>[stun_will_end] / (stun_tick, emit_state(StateTag::ReadyStunned, StateTag::Ready)) = state<Ready>,
        state<ReadyStunned> + event<Beat>                / stun_tick                                                                 = state<ReadyStunned>,

        state<ActingStunned> + event<Beat>[stun_will_end] / (stun_tick, emit_state(StateTag::ActingStunned, StateTag::Acting)) = state<Acting>,
        state<ActingStunned> + event<Beat>                / stun_tick                                                                   = state<ActingStunned>,
        
        // Kill from any live state
        state<Charging>        + event<Killed> / (kill, emit_state(StateTag::Charging, StateTag::Dead))        = state<Dead>,
        state<Ready>           + event<Killed> / (kill, emit_state(StateTag::Ready, StateTag::Dead))           = state<Dead>,
        state<Acting>          + event<Killed> / (kill, emit_state(StateTag::Acting, StateTag::Dead))          = state<Dead>,
        state<ChargingStunned> + event<Killed> / (kill, emit_state(StateTag::ChargingStunned, StateTag::Dead)) = state<Dead>,
        state<ReadyStunned>    + event<Killed> / (kill, emit_state(StateTag::ReadyStunned, StateTag::Dead))    = state<Dead>,
        state<ActingStunned>   + event<Killed> / (kill, emit_state(StateTag::ActingStunned, StateTag::Dead))   = state<Dead>,

        // Dead ignores Beat/Command/Stun (no transitions needed), but can be
        // Revived
        state<Dead> + event<Revived>[revive_full] / (revive, emit_ready, emit_state(StateTag::Dead, StateTag::Ready)) = state<Ready>,
        state<Dead> + event<Revived>              / (revive, emit_state(StateTag::Dead, StateTag::Charging))          = state<Charging>
      );
    // clang-format on
  }
};

} // namespace seerin::atb
