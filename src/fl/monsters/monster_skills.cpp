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
  case MonsterKind::FieldMouse:
    return fl::skills::SkillId::Thump;
  }

  return fl::skills::SkillId::Thump;
}

} // namespace fl::monster
