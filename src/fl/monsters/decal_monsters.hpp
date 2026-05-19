#pragma once

#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

struct DecalMonster {
  static void apply_stormtick_imp(fl::primitives::EntityBuilder &b);
  static void apply_ceiling_grudge(fl::primitives::EntityBuilder &b);
  static void apply_miasma_toad(fl::primitives::EntityBuilder &b);
  static void apply_choir_wisp(fl::primitives::EntityBuilder &b);
  static void apply_gorecap_sprout(fl::primitives::EntityBuilder &b);
  static void apply_rimefang_hare(fl::primitives::EntityBuilder &b);
  static void apply_null_mote(fl::primitives::EntityBuilder &b);
};

void register_decal_monsters();

} // namespace fl::monster
