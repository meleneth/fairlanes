#include "fl/monsters/monster_skills.hpp"

namespace fl::monster {

fl::skills::SkillId known_skill_for(MonsterKind kind) noexcept {
  switch (kind) {
  case MonsterKind::HoneyBadger:
    return fl::skills::SkillId::Eviscerate;
  case MonsterKind::BumpkinHare:
    return fl::skills::SkillId::Bump;
  case MonsterKind::MireSquish:
    return fl::skills::SkillId::Squish;
  case MonsterKind::BarkSmack:
    return fl::skills::SkillId::Smack;
  case MonsterKind::PoisonToad:
    return fl::skills::SkillId::Poison;
  case MonsterKind::Yeti:
    return fl::skills::SkillId::ColdSnap;
  case MonsterKind::Salamander:
    return fl::skills::SkillId::FlameStrike;
  case MonsterKind::FireDrake:
    return fl::skills::SkillId::FlameWave;
  case MonsterKind::StormtickImp:
    return fl::skills::SkillId::Joltspasm;
  case MonsterKind::CeilingGrudge:
    return fl::skills::SkillId::RocksFall;
  case MonsterKind::MiasmaToad:
    return fl::skills::SkillId::SourBreath;
  case MonsterKind::ChoirWisp:
    return fl::skills::SkillId::Mercyburst;
  case MonsterKind::GorecapSprout:
    return fl::skills::SkillId::BloodBloom;
  case MonsterKind::RimefangHare:
    return fl::skills::SkillId::IceSplitter;
  case MonsterKind::NullMote:
    return fl::skills::SkillId::GravitySigh;
  case MonsterKind::FieldMouse:
    return fl::skills::SkillId::Thump;
  }

  return fl::skills::SkillId::Thump;
}

} // namespace fl::monster
