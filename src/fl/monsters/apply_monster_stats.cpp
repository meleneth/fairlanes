#include "fl/monsters/apply_monster_stats.hpp"

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/generated/monster_content.hpp"

namespace fl::monster {

void apply_monster_stats(fl::primitives::EntityBuilder &builder,
                         MonsterKind kind) {
  const auto generated = generated_content::stats(kind);

  auto &stats =
      builder.ctx().reg().get<fl::ecs::components::Stats>(builder.entity());
  stats.name_ = generated.display_name;
  stats.hp_ = generated.hp;
  stats.max_hp_ = generated.hp;
  stats.mp_ = generated.mp;

  if (generated.level > 0) {
    auto &xp =
        builder.ctx().reg().get<fl::ecs::components::TrackXP>(builder.entity());
    xp.level_ = generated.level;
    xp.xp_ = xp.xp_for_level(generated.level);
    xp.next_level_at = xp.xp_for_level(generated.level + 1);
  }
}

} // namespace fl::monster
