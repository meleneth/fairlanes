#pragma once

#include "fl/context.hpp"

namespace fl::ecs::systems {

class PartyGearing {
public:
  static void commit(fl::context::PartyCtx &ctx);
};

} // namespace fl::ecs::systems
