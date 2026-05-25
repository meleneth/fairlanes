#pragma once

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/events/party_bus.hpp"
#include "sr/atb_events.hpp"
#include "sr/timed_scheduler.hpp"

namespace fl::ecs::systems {

class FreezeSystem {
public:
  using Scheduler = seerin::TimedScheduler<seerin::AtbOutEvent>;

  static fl::events::ScopedCombatantListener
  bind_apply_listener(fl::context::PartyCtx &party_ctx,
                      fl::events::CombatantBus &combatant_bus,
                      Scheduler &scheduler);

  static void apply(fl::context::PartyCtx &party_ctx, Scheduler &scheduler,
                    entt::entity source, entt::entity target,
                    int duration_seconds);

  static void shatter(fl::context::PartyCtx &party_ctx, entt::entity source,
                      entt::entity target);

  static void schedule_clear(fl::context::PartyCtx &party_ctx,
                             Scheduler &scheduler, entt::entity target,
                             int clear_after_beats);

  static void clear(fl::context::PartyCtx &party_ctx, entt::entity target);
};

} // namespace fl::ecs::systems
