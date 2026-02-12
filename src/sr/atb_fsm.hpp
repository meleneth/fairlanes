#pragma once

#include <boost/sml.hpp>
#include <entt/entt.hpp>

#include "atb_bus.hpp"
#include "atb_events.hpp"
#include "uWu.hpp"

namespace seerin {

namespace sml = boost::sml;

struct Charging {};
struct Ready {};

struct AtbCtx {
  uWu charge{0};
  uWu max_charge{4800};
};

struct AtbMachine {
  AtbCtx &ctx;
  AtbOutBus &out;
  entt::entity id;

  auto operator()() const {
    using namespace sml;

    const auto will_be_full = [this] {
      return (ctx.charge.v + UWU_PER_BEAT.v) >= ctx.max_charge.v;
    };

    const auto accrue = [this] {
      ctx.charge = uWu{ctx.charge.v + UWU_PER_BEAT.v};
      // out.emit(AtbOutEvent{BecameReady{id}});
    };

    const auto accrue_and_emit_ready = [this] {
      ctx.charge = uWu{ctx.charge.v + UWU_PER_BEAT.v};
      out.emit(AtbOutEvent{BecameReady{id}});
    };

    return make_transition_table(
        *state<Charging> + event<BeatTick>[will_be_full] /
                               accrue_and_emit_ready = state<Ready>,
        state<Charging> + event<BeatTick> / accrue = state<Charging>,

        // NEW: consume the turn, reset, start charging again
        state<Ready> + event<FinishedTurn> / [this] { ctx.charge = uWu{0}; } =
            state<Charging>);
  };
};
} // namespace seerin
