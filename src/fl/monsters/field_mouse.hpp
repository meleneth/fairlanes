#pragma once

#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

class FieldMouse {
public:
  static void apply(fl::primitives::EntityBuilder &b); // defined in .cpp
};

void register_field_mouse();

} // namespace fl::monster
