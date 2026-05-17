#pragma once

#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

class Yeti {
public:
  static void apply(fl::primitives::EntityBuilder &b);
};

void register_yeti();

} // namespace fl::monster
