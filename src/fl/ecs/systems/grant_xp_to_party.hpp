#pragma once
#include <entt/entity/registry.hpp>

#include "fl/context.hpp"

namespace fl::ecs::components {
struct PartyMember;
struct TrackXp;
} // namespace fl::ecs::components

namespace fl::context {
struct EntityCtx;
struct PartyCtx;
} // namespace fl::context

namespace fl::primitives {
struct EncounterData;
}

namespace fl::systems {
using fl::context::EntityCtx;

class GrantXPToParty {
public:
  static void commit_victory(const fl::context::PartyCtx& ctx,
                             const fl::primitives::EncounterData& encounter);
};

} // namespace fl::systems
