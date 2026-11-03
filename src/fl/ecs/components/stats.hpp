#pragma once
#include "fl/primitives/damage.hpp"
#include <string>

namespace fl::ecs::components {

struct Stats {
  std::string name_{"Unknown"};
  int hp_ = 10;
  int max_hp_ = 10;
  int attack_ = 0;
  int armor_ = 0;
  fl::primitives::Resistances resistances_{};

  Stats() = default;
  explicit Stats(std::string name_);
  bool is_alive() const;
};

} // namespace fl::ecs::components
