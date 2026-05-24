#include "decal_monsters.hpp"

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

void DecalMonster::apply_stormtick_imp(EntityBuilder &b) {
  set_stats(b, "Stormtick Imp", 10, 3);
}

void DecalMonster::apply_ceiling_grudge(EntityBuilder &b) {
  set_stats(b, "Ceiling Grudge", 18, 5);
}

void DecalMonster::apply_miasma_toad(EntityBuilder &b) {
  set_stats(b, "Miasma Toad", 15, 4);
}

void DecalMonster::apply_choir_wisp(EntityBuilder &b) {
  set_stats(b, "Choir Wisp", 12, 4);
}

void DecalMonster::apply_gorecap_sprout(EntityBuilder &b) {
  set_stats(b, "Gorecap Sprout", 16, 5);
}

void DecalMonster::apply_rimefang_hare(EntityBuilder &b) {
  set_stats(b, "Rimefang Hare", 13, 4);
}

void DecalMonster::apply_null_mote(EntityBuilder &b) {
  set_stats(b, "Null Mote", 14, 5);
}

void register_decal_monsters() {
  register_monster(MonsterKind::StormtickImp, [](EntityBuilder &b) {
    DecalMonster::apply_stormtick_imp(b);
  },
                   {fl::skills::SkillId::Joltspasm});
  register_monster(MonsterKind::CeilingGrudge, [](EntityBuilder &b) {
    DecalMonster::apply_ceiling_grudge(b);
  },
                   {fl::skills::SkillId::RocksFall});
  register_monster(MonsterKind::MiasmaToad, [](EntityBuilder &b) {
    DecalMonster::apply_miasma_toad(b);
  },
                   {fl::skills::SkillId::SourBreath});
  register_monster(MonsterKind::ChoirWisp,
                   [](EntityBuilder &b) { DecalMonster::apply_choir_wisp(b); },
                   {fl::skills::SkillId::Mercyburst});
  register_monster(MonsterKind::GorecapSprout, [](EntityBuilder &b) {
    DecalMonster::apply_gorecap_sprout(b);
  },
                   {fl::skills::SkillId::BloodBloom});
  register_monster(MonsterKind::RimefangHare, [](EntityBuilder &b) {
    DecalMonster::apply_rimefang_hare(b);
  },
                   {fl::skills::SkillId::IceSplitter});
  register_monster(MonsterKind::NullMote,
                   [](EntityBuilder &b) { DecalMonster::apply_null_mote(b); },
                   {fl::skills::SkillId::GravitySigh});
}

} // namespace fl::monster
