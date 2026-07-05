#include "honey_badger.hpp"

#include "fl/monsters/apply_monster_stats.hpp"
#include "fl/monsters/monster_registry.hpp"
#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

using fl::primitives::EntityBuilder;

void HoneyBadger::apply(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::HoneyBadger);
}

void register_honey_badger() {
  register_monster(fl::monster::MonsterKind::HoneyBadger,
                   [](EntityBuilder &b) { HoneyBadger::apply(b); });
}

} // namespace fl::monster
