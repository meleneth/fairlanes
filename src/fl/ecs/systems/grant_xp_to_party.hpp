#pragma once
#include <entt/entt.hpp>

#include "fl/context.hpp"

namespace fl::ecs::components {
struct PartyMember;
struct TrackXp;
} // namespace fl::ecs::components

namespace fl::context {
struct EntityCtx;
}

namespace fl::systems {
using fl::context::EntityCtx;

class GrantXPToParty {
public:
  static void commit(const fl::context::EntityCtx &ctx, int amount);
};

} // namespace fl::systems
