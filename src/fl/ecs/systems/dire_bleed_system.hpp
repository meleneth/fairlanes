#pragma once

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "sr/atb_events.hpp"
#include "sr/timed_scheduler.hpp"

namespace fl::ecs::systems {

class DireBleedSystem {
public:
  using Scheduler = seerin::TimedScheduler<seerin::AtbOutEvent>;

  static void apply(fl::context::PartyCtx &party_ctx, Scheduler &scheduler,
                    entt::entity source, entt::entity target);

  static void bind_cleanup_and_schedule(fl::context::PartyCtx &party_ctx,
                                        Scheduler &scheduler,
                                        entt::entity target);

  static void schedule_tick(fl::context::PartyCtx &party_ctx,
                            Scheduler &scheduler, entt::entity target);

  static void clear(fl::context::PartyCtx &party_ctx, entt::entity target);
};

} // namespace fl::ecs::systems
