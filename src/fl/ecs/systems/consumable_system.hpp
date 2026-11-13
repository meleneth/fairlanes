#pragma once

#include <cstdint>
#include <optional>

#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>

#include "fl/consumables/consumable_kind.hpp"
#include "fl/context.hpp"

namespace fl::ecs::systems {

class ConsumableSystem {
public:
  static entt::entity create(entt::registry& reg, fl::consumables::ConsumableKind kind,
                             int quantity = 1);
  static void grant_starter_pack(fl::context::PartyCtx& ctx);
  static std::optional<entt::entity> choose_combat_potion(fl::context::EncounterCtx& ctx,
                                                          entt::entity actor);
  static bool use_potion(fl::context::EncounterCtx& ctx, entt::entity actor, entt::entity item);
  static bool prepare_flask(fl::context::PartyCtx& ctx, entt::entity actor, entt::entity item,
                            std::uint64_t world_beat);
  static void expire_timed_buffs(entt::registry& reg, std::uint64_t world_beat);
};

} // namespace fl::ecs::systems
