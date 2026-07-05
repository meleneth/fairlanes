#include "honey_badger.hpp"

#include "fl/ecs/components/stats.hpp"
#include "fl/monsters/monster_registry.hpp"
#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

using fl::ecs::components::Stats;
using fl::primitives::EntityBuilder;

void HoneyBadger::apply(EntityBuilder &b) {
  auto &s = b.ctx().reg().get<Stats>(b.entity());
  s.name_ = "Honey Badger";
  s.hp_ = 500;
  s.max_hp_ = 500;
  s.mp_ = 0;
}

void register_honey_badger() {
  register_monster(fl::monster::MonsterKind::HoneyBadger,
                   [](EntityBuilder &b) { HoneyBadger::apply(b); });
}

} // namespace fl::monster
