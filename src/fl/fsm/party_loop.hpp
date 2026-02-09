#pragma once
#include <boost/sml.hpp>

#include "fl/context.hpp"

namespace fl::fsm {
namespace sml = boost::sml;

struct NextEvent {};

struct PartyLoop {
  // Put all behavior in a ctx-only “ops” namespace/struct.
  struct Ops {
    static void enter_idle(fl::context::PartyCtx &ctx);
    static void enter_farming(fl::context::PartyCtx &ctx);
    static void exit_farming(fl::context::PartyCtx &ctx);
    static void enter_fixing(fl::context::PartyCtx &ctx);

    static void combat_tick(fl::context::PartyCtx &ctx);
    static void next_event(fl::context::PartyCtx &ctx);

    static bool needs_town(fl::context::PartyCtx &ctx);
    static bool in_combat(fl::context::PartyCtx &ctx);
  };
  auto operator()() const {
    using namespace sml;

    struct Idle {};
    struct Farming {};
    struct Fixing {};
    struct CombatIdle {};

    // Actions
    const auto enter_idle = [](fl::context::PartyCtx &ctx) {
      Ops::enter_idle(ctx);
    };
    const auto enter_farming = [](fl::context::PartyCtx &ctx) {
      Ops::enter_farming(ctx);
    };
    const auto exit_farming = [](fl::context::PartyCtx &ctx) {
      Ops::exit_farming(ctx);
    };
    const auto enter_fixing = [](fl::context::PartyCtx &ctx) {
      Ops::enter_fixing(ctx);
    };
    const auto combat_tick = [](fl::context::PartyCtx &ctx) {
      Ops::combat_tick(ctx);
    };

    // Guards
    const auto needs_town = [](fl::context::PartyCtx &ctx) {
      return Ops::needs_town(ctx);
    };
    const auto in_combat = [](fl::context::PartyCtx &ctx) {
      return Ops::in_combat(ctx);
    };

    return make_transition_table(
        *state<Idle> + sml::on_entry<_> / enter_idle,
        state<Farming> + sml::on_entry<_> / enter_farming,
        state<Farming> + sml::on_exit<_> / exit_farming,
        state<Fixing> + sml::on_entry<_> / enter_fixing,

        state<Idle> + event<NextEvent> = state<Farming>,
        state<Farming> + event<NextEvent>[needs_town] = state<Fixing>,
        state<Farming> + event<NextEvent>[in_combat] / combat_tick,
        state<Farming> + event<NextEvent> = state<CombatIdle>,
        state<CombatIdle> + event<NextEvent> = state<Farming>,
        state<Fixing> + event<NextEvent> = state<Idle>);
  }
};

} // namespace fl::fsm
