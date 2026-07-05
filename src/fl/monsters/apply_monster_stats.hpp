#pragma once

#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

void apply_monster_stats(fl::primitives::EntityBuilder &builder,
                         MonsterKind kind);

} // namespace fl::monster
