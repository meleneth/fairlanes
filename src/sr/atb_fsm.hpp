#pragma once

#include <boost/sml.hpp>
#include <entt/entt.hpp>

#include "atb_bus.hpp"
#include "atb_events.hpp"
#include "fl/ecs/components/atb_charge.hpp"

namespace seerin {

namespace sml = boost::sml;

struct Charging {};
struct Ready {};
struct FrozenState {};

struct AtbMachine {
  entt::registry &reg;
  AtbOutBus &out;
  entt::entity id;

  auto operator()() const {
    using namespace sml;

    const auto will_be_full = [this] {
      auto &ctx = reg.get<fl::ecs::components::AtbCharge>(id);
      return (ctx.charge + 80) >= ctx.max_charge;
    };

    const auto accrue = [this] {
      auto &ctx = reg.get<fl::ecs::components::AtbCharge>(id);
      ctx.charge += 80;
      // out.emit(AtbOutEvent{BecameReady{id}});
    };

    const auto accrue_and_emit_ready = [this] {
      auto &ctx = reg.get<fl::ecs::components::AtbCharge>(id);
      ctx.charge += 80;
      out.emit(AtbOutEvent{BecameReady{id}});
    };

    const auto is_full = [this] {
      auto &ctx = reg.get<fl::ecs::components::AtbCharge>(id);
      return ctx.charge >= ctx.max_charge;
    };

    const auto emit_ready = [this] { out.emit(AtbOutEvent{BecameReady{id}}); };

    return make_transition_table(
        *state<Charging> + event<BeatTick>[will_be_full] /
                               accrue_and_emit_ready = state<Ready>,
        state<Charging> + event<BeatTick> / accrue = state<Charging>,

        state<Charging> + event<Frozen> = state<FrozenState>,
        state<Ready> + event<Frozen> = state<FrozenState>,
        state<FrozenState> + event<BeatTick> = state<FrozenState>,
        state<FrozenState> + event<Thawed>[is_full] / emit_ready = state<Ready>,
        state<FrozenState> + event<Thawed> = state<Charging>,

        // NEW: consume the turn, reset, start charging again
        state<Ready> + event<FinishedTurn> /
                           [this] {
                             auto &ctx =
                                 reg.get<fl::ecs::components::AtbCharge>(id);
                             ctx.charge = 0;
                           } = state<Charging>,
        state<FrozenState> +
            event<FinishedTurn> /
                [this] {
                  auto &ctx = reg.get<fl::ecs::components::AtbCharge>(id);
                  ctx.charge = 0;
                } = state<Charging>);
  };
};
} // namespace seerin
