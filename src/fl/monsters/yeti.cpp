#include "yeti.hpp"

#include "fl/ecs/components/stats.hpp"
#include "fl/monsters/monster_registry.hpp"
#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

using fl::ecs::components::Stats;
using fl::primitives::EntityBuilder;

void Yeti::apply(EntityBuilder &b) {
  auto &s = b.ctx().reg().get<Stats>(b.entity());
  s.name_ = "Yeti";
  s.hp_ = 40;
  s.max_hp_ = 40;
  s.mp_ = 0;
}

void register_yeti() {
  register_monster(fl::monster::MonsterKind::Yeti,
                   [](EntityBuilder &b) { Yeti::apply(b); });
}

} // namespace fl::monster
