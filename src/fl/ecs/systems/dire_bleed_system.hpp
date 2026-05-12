#pragma once

#include <functional>

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "sr/atb_events.hpp"
#include "sr/timed_scheduler.hpp"

namespace fl::ecs::systems {

class DireBleedSystem {
public:
  using Scheduler = seerin::TimedScheduler<seerin::AtbOutEvent>;
  using ClearPendingFn = std::function<void(entt::entity)>;

  static void bind_cleanup_and_schedule(fl::context::PartyCtx &party_ctx,
                                        Scheduler &scheduler,
                                        entt::entity target,
                                        ClearPendingFn clear_pending);

  static void schedule_tick(fl::context::PartyCtx &party_ctx,
                            Scheduler &scheduler, entt::entity target,
                            ClearPendingFn clear_pending);

  static void clear(fl::context::PartyCtx &party_ctx, entt::entity target,
                    const ClearPendingFn &clear_pending);
};

} // namespace fl::ecs::systems
