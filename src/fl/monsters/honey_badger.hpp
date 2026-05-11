#pragma once

#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

class HoneyBadger {
public:
  static void apply(fl::primitives::EntityBuilder &b);
};

void register_honey_badger();

} // namespace fl::monster
