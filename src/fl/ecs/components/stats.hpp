#pragma once
#include <entt/entt.hpp>

#include "fl/primitives/damage.hpp"

namespace fl::widgets {
class FancyLog;
}
namespace fl::fsm {
class PartyLoopCtx;
}

namespace fl::ecs::components {

using fl::primitives::Damage;
using fl::primitives::Resistances;

struct Stats {
  std::string name_{"Unknown"};
  int hp_ = 10;
  int max_hp_ = 10;
  int mp_ = 0;
  int max_mp_ = 0;
  Resistances resistances_;

  Stats() = default;
  explicit Stats(std::string name_);
  bool is_alive();
};

} // namespace fl::ecs::components
