#pragma once

#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

class FireDrake {
public:
  static void apply(fl::primitives::EntityBuilder &b);
};

void register_fire_drake();

} // namespace fl::monster
