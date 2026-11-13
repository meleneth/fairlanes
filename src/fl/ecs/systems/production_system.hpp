#pragma once

#include <cstdint>

#include "fl/context.hpp"

namespace fl::ecs::systems {

class ProductionSystem {
public:
  static void evaluate_objective(fl::context::AccountCtx ctx);
  static void advance(fl::context::PartyCtx ctx, std::uint64_t world_beat);
};

} // namespace fl::ecs::systems
