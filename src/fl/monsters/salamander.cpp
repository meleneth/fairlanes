#include "salamander.hpp"

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_registry.hpp"

namespace fl::monster {

using fl::ecs::components::Stats;
using fl::ecs::components::TrackXP;
using fl::primitives::EntityBuilder;

void Salamander::apply(EntityBuilder &b) {
  auto &s = b.ctx().reg().get<Stats>(b.entity());
  s.name_ = "Salamander";
  s.hp_ = 24;
  s.max_hp_ = 24;
  s.mp_ = 0;

  auto &xp = b.ctx().reg().get<TrackXP>(b.entity());
  xp.level_ = 6;
  xp.xp_ = xp.xp_for_level(xp.level_);
  xp.next_level_at = xp.xp_for_level(xp.level_ + 1);
}

void register_salamander() {
  register_monster(fl::monster::MonsterKind::Salamander,
                   [](EntityBuilder &b) { Salamander::apply(b); });
}

} // namespace fl::monster
