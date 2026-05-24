#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kSquishTags{SkillTag::Physical, SkillTag::Blunt,
                                 SkillTag::Control, SkillTag::Melee};
constexpr SkillDefinition kSquishDefinition{
    SkillId::Squish,
    "Squish",
    std::span<const SkillTag>{kSquishTags.data(), kSquishTags.size()},
    20,
    0,
    SkillExecutionKind::ThumpLike,
    std::nullopt,
};

} // namespace

const SkillDefinition &squish_skill_definition() noexcept {
  return kSquishDefinition;
}

} // namespace fl::skills
