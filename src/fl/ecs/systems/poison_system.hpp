#pragma once

#include <functional>

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/events/party_bus.hpp"
#include "sr/atb_events.hpp"
#include "sr/timed_scheduler.hpp"

namespace fl::ecs::systems {

class PoisonSystem {
public:
  using Scheduler = seerin::TimedScheduler<seerin::AtbOutEvent>;
  using ClearPendingFn = std::function<void(entt::entity)>;

  static fl::events::ScopedCombatantListener
  bind_apply_listener(fl::context::PartyCtx &party_ctx,
                      fl::events::CombatantBus &combatant_bus,
                      Scheduler &scheduler, ClearPendingFn clear_pending);

  static void apply(fl::context::PartyCtx &party_ctx, Scheduler &scheduler,
                    entt::entity source, entt::entity target,
                    int damage_per_tick, int duration_seconds,
                    ClearPendingFn clear_pending);

  static void schedule_tick(fl::context::PartyCtx &party_ctx,
                            Scheduler &scheduler, entt::entity target,
                            ClearPendingFn clear_pending);

  static void schedule_visual_pulse(fl::context::PartyCtx &party_ctx,
                                    Scheduler &scheduler, entt::entity target,
                                    bool bright);

  static void clear(fl::context::PartyCtx &party_ctx, entt::entity target,
                    const ClearPendingFn &clear_pending);
};

} // namespace fl::ecs::systems
