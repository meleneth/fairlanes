#include "woodland_critter.hpp"

#include <string>
#include <utility>

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_registry.hpp"

namespace fl::monster {

using fl::ecs::components::Stats;
using fl::ecs::components::TrackXP;
using fl::primitives::EntityBuilder;

namespace {

void set_stats(EntityBuilder &b, std::string name, int hp, int level) {
  auto &s = b.ctx().reg().get<Stats>(b.entity());
  s.name_ = std::move(name);
  s.hp_ = hp;
  s.max_hp_ = hp;
  s.mp_ = 0;

  auto &xp = b.ctx().reg().get<TrackXP>(b.entity());
  xp.level_ = level;
  xp.xp_ = xp.xp_for_level(level);
  xp.next_level_at = xp.xp_for_level(level + 1);
}

} // namespace

void WoodlandCritter::apply_bumpkin(EntityBuilder &b) {
  set_stats(b, "Bumpkin Hare", 7, 2);
}

void WoodlandCritter::apply_mire_squish(EntityBuilder &b) {
  set_stats(b, "Mire Squish", 9, 3);
}

void WoodlandCritter::apply_bark_smack(EntityBuilder &b) {
  set_stats(b, "Bark Smack", 12, 4);
}

void register_woodland_critters() {
  register_monster(fl::monster::MonsterKind::BumpkinHare,
                   [](EntityBuilder &b) { WoodlandCritter::apply_bumpkin(b); });
  register_monster(fl::monster::MonsterKind::MireSquish, [](EntityBuilder &b) {
    WoodlandCritter::apply_mire_squish(b);
  });
  register_monster(fl::monster::MonsterKind::BarkSmack, [](EntityBuilder &b) {
    WoodlandCritter::apply_bark_smack(b);
  });
}

} // namespace fl::monster
