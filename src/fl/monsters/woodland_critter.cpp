#include "woodland_critter.hpp"

#include "fl/monsters/apply_monster_stats.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_registry.hpp"

namespace fl::monster {

using fl::primitives::EntityBuilder;

void WoodlandCritter::apply_bumpkin(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::BumpkinHare);
}

void WoodlandCritter::apply_scaredy_cat(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::ScaredyCat);
}

void WoodlandCritter::apply_mire_squish(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::MireSquish);
}

void WoodlandCritter::apply_bark_smack(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::BarkSmack);
}

void WoodlandCritter::apply_poison_toad(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::PoisonToad);
}

void register_woodland_critters() {
  register_monster(fl::monster::MonsterKind::BumpkinHare,
                   [](EntityBuilder &b) { WoodlandCritter::apply_bumpkin(b); });
  register_monster(fl::monster::MonsterKind::ScaredyCat,
                   [](EntityBuilder &b) {
                     WoodlandCritter::apply_scaredy_cat(b);
                   });
  register_monster(fl::monster::MonsterKind::MireSquish, [](EntityBuilder &b) {
    WoodlandCritter::apply_mire_squish(b);
  });
  register_monster(fl::monster::MonsterKind::BarkSmack, [](EntityBuilder &b) {
    WoodlandCritter::apply_bark_smack(b);
  });
  register_monster(fl::monster::MonsterKind::PoisonToad, [](EntityBuilder &b) {
    WoodlandCritter::apply_poison_toad(b);
  });
}

} // namespace fl::monster
