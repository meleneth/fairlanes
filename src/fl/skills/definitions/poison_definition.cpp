#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kPoisonTags{SkillTag::Poison, SkillTag::Disease};
constexpr SkillDefinition kPoisonDefinition{
    SkillId::Poison,
    "Poison",
    std::span<const SkillTag>{kPoisonTags.data(), kPoisonTags.size()},
    5,
    0,
    SkillExecutionKind::Poison,
    std::nullopt,
};

} // namespace

const SkillDefinition &poison_skill_definition() noexcept {
  return kPoisonDefinition;
}

} // namespace fl::skills
