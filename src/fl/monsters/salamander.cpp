#include "salamander.hpp"

#include "fl/monsters/apply_monster_stats.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_registry.hpp"

namespace fl::monster {

using fl::primitives::EntityBuilder;

void Salamander::apply(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::Salamander);
}

void register_salamander() {
  register_monster(fl::monster::MonsterKind::Salamander,
                   [](EntityBuilder &b) { Salamander::apply(b); });
}

} // namespace fl::monster
