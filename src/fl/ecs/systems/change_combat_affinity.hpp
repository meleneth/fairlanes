#pragma once

#include <entt/entity/entity.hpp>

#include "fl/context.hpp"
#include "fl/primitives/combat_affinity.hpp"

namespace fl::ecs::systems {

enum class AffinityChangeResult { Changed, Unchanged, NotAMember, NotInTown };

class ChangeCombatAffinity {
public:
  static AffinityChangeResult commit(fl::context::PartyCtx& ctx, entt::entity member,
                                     fl::primitives::CombatAffinity affinity);
};

} // namespace fl::ecs::systems
