#pragma once
#include <boost/sml.hpp>
#include <entt/entt.hpp>
#include <spdlog/spdlog.h>

#include "fl/context.hpp"
#include "fl/ecs/systems/grant_xp_to_party.hpp"
#include "fl/fsm/party_loop_ctx.hpp"
#include "fl/primitives/encounter_builder.hpp"

namespace fl::fsm {
namespace sml = boost::sml;

struct NextEvent {};

struct PartyLoop {
  PartyLoop(PartyLoopCtx &context) : ctx_(context) {}

  void enter_idle(fl::fsm::PartyLoopCtx &ctx);
  void enter_farming(fl::fsm::PartyLoopCtx &ctx);
  void exit_farming(fl::fsm::PartyLoopCtx &ctx);
  void enter_fixing(fl::fsm::PartyLoopCtx &ctx);

  void combat_tick(fl::fsm::PartyLoopCtx &ctx);
  void next_event(fl::fsm::PartyLoopCtx &ctx);

  bool needs_town(fl::fsm::PartyLoopCtx &ctx);
  bool in_combat(fl::fsm::PartyLoopCtx &ctx);

  auto operator()() const {
    using namespace sml;
    using namespace fl::ecs::components;

    struct Idle {};
    struct Farming {};
    struct Fixing {};
    struct CombatIdle {};

    return make_transition_table(
        *state<Idle> + on_entry<_> / &PartyLoop::enter_idle,
        state<Farming> + on_entry<_> / &PartyLoop::enter_farming,
        state<Farming> + sml::on_exit<_> / &PartyLoop::exit_farming,
        state<Fixing> + on_entry<_> / &PartyLoop::enter_fixing,

        state<Idle> + event<NextEvent> = state<Farming>,
        state<Farming> + event<NextEvent>[&PartyLoop::needs_town] =
            state<Fixing>,
        state<Farming> +
            event<NextEvent>[&PartyLoop::in_combat] / &PartyLoop::combat_tick,
        state<Farming> + event<NextEvent> = state<CombatIdle>,
        state<CombatIdle> + event<NextEvent> = state<Farming>,
        state<Fixing> + event<NextEvent> = state<Idle>);
  }

private:
  PartyLoopCtx &ctx_;
};

} // namespace fl::fsm
