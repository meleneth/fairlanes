#pragma once

#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

class Salamander {
public:
  static void apply(fl::primitives::EntityBuilder &b);
};

void register_salamander();

} // namespace fl::monster
