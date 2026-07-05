#include "fire_drake.hpp"

#include "fl/monsters/apply_monster_stats.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_registry.hpp"

namespace fl::monster {

using fl::primitives::EntityBuilder;

void FireDrake::apply(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::FireDrake);
}

void register_fire_drake() {
  register_monster(fl::monster::MonsterKind::FireDrake,
                   [](EntityBuilder &b) { FireDrake::apply(b); });
}

} // namespace fl::monster
