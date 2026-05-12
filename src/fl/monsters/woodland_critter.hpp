#pragma once

#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

class WoodlandCritter {
public:
  static void apply_bumpkin(fl::primitives::EntityBuilder &b);
  static void apply_mire_squish(fl::primitives::EntityBuilder &b);
  static void apply_bark_smack(fl::primitives::EntityBuilder &b);
};

void register_woodland_critters();

} // namespace fl::monster
