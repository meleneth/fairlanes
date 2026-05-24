#include "fire_drake.hpp"

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_registry.hpp"

namespace fl::monster {

using fl::ecs::components::Stats;
using fl::ecs::components::TrackXP;
using fl::primitives::EntityBuilder;

void FireDrake::apply(EntityBuilder &b) {
  auto &s = b.ctx().reg().get<Stats>(b.entity());
  s.name_ = "Fire Drake";
  s.hp_ = 500;
  s.max_hp_ = 500;
  s.mp_ = 0;

  auto &xp = b.ctx().reg().get<TrackXP>(b.entity());
  xp.level_ = 8;
  xp.xp_ = xp.xp_for_level(xp.level_);
  xp.next_level_at = xp.xp_for_level(xp.level_ + 1);
}

void register_fire_drake() {
  register_monster(fl::monster::MonsterKind::FireDrake,
                   [](EntityBuilder &b) { FireDrake::apply(b); },
                   {fl::skills::SkillId::FlameWave});
}

} // namespace fl::monster
