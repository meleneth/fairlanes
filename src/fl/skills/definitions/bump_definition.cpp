#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kBumpTags{SkillTag::Physical, SkillTag::Blunt,
                               SkillTag::Melee};
constexpr SkillDefinition kBumpDefinition{
    SkillId::Bump,
    "Bump",
    std::span<const SkillTag>{kBumpTags.data(), kBumpTags.size()},
    20,
    SkillExecutionKind::ThumpLike,
    std::nullopt,
};

} // namespace

const SkillDefinition &bump_skill_definition() noexcept {
  return kBumpDefinition;
}

} // namespace fl::skills
