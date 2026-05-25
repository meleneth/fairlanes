#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <utility>

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/ecs/components/status_effect.hpp"
#include "fl/events/party_bus.hpp"
#include "sr/atb_events.hpp"
#include "sr/timed_scheduler.hpp"

namespace fl::ecs::systems {

class StatusEffectLifetime {
public:
  using Scheduler = seerin::TimedScheduler<seerin::AtbOutEvent>;

  static fl::ecs::components::StatusEffectInstance
  create_instance(fl::context::PartyCtx &party_ctx, entt::entity owner);

  StatusEffectLifetime(fl::context::PartyCtx &party_ctx, Scheduler &scheduler,
                       fl::ecs::components::StatusEffectInstance &instance);

  [[nodiscard]] entt::entity owner() const noexcept;
  [[nodiscard]] entt::entity effect_id() const noexcept;

  template <class Fn> void on_owner_died(Fn &&fn) {
    const auto owner_id = instance_.owner;
    instance_.owner_died_sub = fl::events::ScopedCombatantListener{
        owner_bus(), std::in_place_type<fl::events::PlayerDied>,
        [owner_id,
         fn = std::forward<Fn>(fn)](const fl::events::PlayerDied &ev) mutable {
          if (ev.player == owner_id) {
            fn(ev);
          }
        }};
  }

  template <class Fn> void on_combat_removed(Fn &&fn) {
    auto shared_fn = std::make_shared<std::decay_t<Fn>>(std::forward<Fn>(fn));
    instance_.party_left_combat_sub = fl::events::ScopedPartyListener{
        party_ctx_.bus(), std::in_place_type<fl::events::PartyLeftCombat>,
        [shared_fn](const fl::events::PartyLeftCombat &ev) mutable {
          (*shared_fn)(ev);
        }};
    instance_.party_wiped_sub = fl::events::ScopedPartyListener{
        party_ctx_.bus(), std::in_place_type<fl::events::PartyWiped>,
        [shared_fn](const fl::events::PartyWiped &ev) mutable {
          (*shared_fn)(ev);
        }};
  }

  void schedule_in_beats(int beats, std::string_view note,
                         std::function<void()> fn);
  void clear_scheduled();
  void destroy_instance_entity();

private:
  fl::events::CombatantBus &owner_bus();

  fl::context::PartyCtx &party_ctx_;
  Scheduler &scheduler_;
  fl::ecs::components::StatusEffectInstance &instance_;
};

} // namespace fl::ecs::systems
