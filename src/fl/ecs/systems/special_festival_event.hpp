#pragma once

#include <cstddef>

#include "fl/context.hpp"

namespace fl::ecs::systems {

class SpecialFestivalEvent {
public:
  static constexpr std::size_t kDropsPerParty = 500;

  static void grant_starting_drops(fl::context::PartyCtx &ctx);
};

} // namespace fl::ecs::systems
