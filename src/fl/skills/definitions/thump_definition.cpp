#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kThumpTags{SkillTag::Physical, SkillTag::Blunt,
                                SkillTag::Melee};
constexpr SkillDefinition kThumpDefinition{
    SkillId::Thump,
    "Thump",
    std::span<const SkillTag>{kThumpTags.data(), kThumpTags.size()},
    20,
    SkillExecutionKind::ThumpLike,
    std::nullopt,
};

} // namespace

const SkillDefinition &thump_skill_definition() noexcept {
  return kThumpDefinition;
}

} // namespace fl::skills
