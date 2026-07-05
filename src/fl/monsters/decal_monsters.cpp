#include "decal_monsters.hpp"

#include "fl/monsters/apply_monster_stats.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_registry.hpp"

namespace fl::monster {

using fl::primitives::EntityBuilder;

void DecalMonster::apply_stormtick_imp(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::StormtickImp);
}

void DecalMonster::apply_ceiling_grudge(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::CeilingGrudge);
}

void DecalMonster::apply_miasma_toad(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::MiasmaToad);
}

void DecalMonster::apply_choir_wisp(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::ChoirWisp);
}

void DecalMonster::apply_gorecap_sprout(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::GorecapSprout);
}

void DecalMonster::apply_rimefang_hare(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::RimefangHare);
}

void DecalMonster::apply_null_mote(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::NullMote);
}

void register_decal_monsters() {
  register_monster(MonsterKind::StormtickImp, [](EntityBuilder &b) {
    DecalMonster::apply_stormtick_imp(b);
  });
  register_monster(MonsterKind::CeilingGrudge, [](EntityBuilder &b) {
    DecalMonster::apply_ceiling_grudge(b);
  });
  register_monster(MonsterKind::MiasmaToad, [](EntityBuilder &b) {
    DecalMonster::apply_miasma_toad(b);
  });
  register_monster(MonsterKind::ChoirWisp,
                   [](EntityBuilder &b) { DecalMonster::apply_choir_wisp(b); });
  register_monster(MonsterKind::GorecapSprout, [](EntityBuilder &b) {
    DecalMonster::apply_gorecap_sprout(b);
  });
  register_monster(MonsterKind::RimefangHare, [](EntityBuilder &b) {
    DecalMonster::apply_rimefang_hare(b);
  });
  register_monster(MonsterKind::NullMote,
                   [](EntityBuilder &b) { DecalMonster::apply_null_mote(b); });
}

} // namespace fl::monster
