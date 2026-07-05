#include "yeti.hpp"

#include "fl/monsters/apply_monster_stats.hpp"
#include "fl/monsters/monster_registry.hpp"
#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

using fl::primitives::EntityBuilder;

void Yeti::apply(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::Yeti);
}

void register_yeti() {
  register_monster(fl::monster::MonsterKind::Yeti,
                   [](EntityBuilder &b) { Yeti::apply(b); });
}

} // namespace fl::monster
