#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kSmackTags{SkillTag::Physical, SkillTag::Blunt,
                                SkillTag::Melee};
constexpr SkillDefinition kSmackDefinition{
    SkillId::Smack,
    "Smack",
    std::span<const SkillTag>{kSmackTags.data(), kSmackTags.size()},
    20,
    SkillExecutionKind::ThumpLike,
    std::nullopt,
};

} // namespace

const SkillDefinition &smack_skill_definition() noexcept {
  return kSmackDefinition;
}

} // namespace fl::skills
