#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kRocksFallTags{SkillTag::Physical, SkillTag::Earth,
                                    SkillTag::Blunt, SkillTag::Area};
constexpr SkillDefinition kRocksFallDefinition{
    SkillId::RocksFall,
    "Rocks Fall",
    std::span<const SkillTag>{kRocksFallTags.data(), kRocksFallTags.size()},
    5,
    0,
    SkillExecutionKind::DecalStrike,
    fl::widgets::effects::DecalAnimationKind::RocksFall,
};

} // namespace

const SkillDefinition &rocks_fall_skill_definition() noexcept {
  return kRocksFallDefinition;
}

} // namespace fl::skills
