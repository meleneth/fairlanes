#pragma once

#include <cstdint>
#include <vector>

#include "fl/consumables/consumable_kind.hpp"

namespace fl::ecs::components {

struct TimedBuff {
  fl::consumables::TimedBuffKind kind{fl::consumables::TimedBuffKind::Attack};
  int magnitude{0};
  std::uint64_t applied_world_beat{0};
  std::uint64_t expires_world_beat{0};
  std::uint64_t remaining_beats{0};
};

struct TimedBuffs {
  std::vector<TimedBuff> effects;
};

} // namespace fl::ecs::components
