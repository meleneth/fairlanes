#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kFlameStrikeTags{SkillTag::Fire, SkillTag::Projectile};
constexpr SkillDefinition kFlameStrikeDefinition{
    SkillId::FlameStrike,
    "Flame Strike",
    std::span<const SkillTag>{kFlameStrikeTags.data(),
                              kFlameStrikeTags.size()},
    5,
    SkillExecutionKind::FlameStrike,
    fl::widgets::effects::DecalAnimationKind::FlameWave,
};

} // namespace

const SkillDefinition &flame_strike_skill_definition() noexcept {
  return kFlameStrikeDefinition;
}

} // namespace fl::skills
