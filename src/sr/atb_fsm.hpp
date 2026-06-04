#pragma once

#include <algorithm>
#include <cstdint>

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

  static constexpr int kChargePerBeat{80};

  static int64_t charge_per_beat(entt::registry &reg, entt::entity id) {
    return std::max<int64_t>(0, charge(reg, id).charge_per_beat);
  }

  static fl::ecs::components::AtbCharge &charge(entt::registry &reg,
                                                entt::entity id) {
    return reg.get<fl::ecs::components::AtbCharge>(id);
  }

  struct WillBeFull {
    entt::registry &reg;
    entt::entity id;

    bool operator()() const {
      const auto &ctx = charge(reg, id);
      return (ctx.charge + charge_per_beat(reg, id)) >= ctx.max_charge;
    }
  };

  struct Accrue {
    entt::registry &reg;
    entt::entity id;

    void operator()() const { charge(reg, id).charge += charge_per_beat(reg, id); }
  };

  struct AccrueAndEmitReady {
    entt::registry &reg;
    AtbOutBus &out;
    entt::entity id;

    void operator()() const {
      charge(reg, id).charge += charge_per_beat(reg, id);
      out.emit(AtbOutEvent{BecameReady{id}});
    }
  };

  struct IsFull {
    entt::registry &reg;
    entt::entity id;

    bool operator()() const {
      const auto &ctx = charge(reg, id);
      return ctx.charge >= ctx.max_charge;
    }
  };

  struct EmitReady {
    AtbOutBus &out;
    entt::entity id;

    void operator()() const { out.emit(AtbOutEvent{BecameReady{id}}); }
  };

  struct ResetCharge {
    entt::registry &reg;
    entt::entity id;

    void operator()() const { charge(reg, id).charge = 0; }
  };

  auto operator()() const {
    using namespace sml;

    return make_transition_table(
        *state<Charging> + event<BeatTick>[WillBeFull{reg, id}] /
                               AccrueAndEmitReady{reg, out, id} = state<Ready>,
        state<Charging> + event<BeatTick> / Accrue{reg, id} = state<Charging>,

        state<Charging> + event<Frozen> = state<FrozenState>,
        state<Ready> + event<Frozen> = state<FrozenState>,
        state<FrozenState> + event<BeatTick> = state<FrozenState>,
        state<FrozenState> +
            event<Thawed>[IsFull{reg, id}] / EmitReady{out, id} = state<Ready>,
        state<FrozenState> + event<Thawed> = state<Charging>,

        state<Ready> + event<FinishedTurn> / ResetCharge{reg, id} =
            state<Charging>,
        state<FrozenState> + event<FinishedTurn> / ResetCharge{reg, id} =
            state<Charging>);
  };
};
} // namespace seerin
