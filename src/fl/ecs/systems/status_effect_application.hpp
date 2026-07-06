#pragma once

#include <functional>
#include <utility>

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/ecs/systems/status_effect_lifetime.hpp"

namespace fl::ecs::systems {

template <typename Status, typename ClearExisting, typename Configure>
Status &
replace_status_effect(fl::context::PartyCtx &party_ctx, entt::entity target,
                      ClearExisting &&clear_existing, Configure &&configure) {
  auto &reg = party_ctx.reg();

  if (reg.any_of<Status>(target)) {
    std::invoke(std::forward<ClearExisting>(clear_existing), party_ctx, target);
  }

  auto &status = reg.emplace<Status>(target);
  status.effect = StatusEffectLifetime::create_instance(party_ctx, target);
  std::invoke(std::forward<Configure>(configure), status);
  return status;
}

} // namespace fl::ecs::systems
