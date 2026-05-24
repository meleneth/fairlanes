#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kJoltspasmTags{SkillTag::Lightning, SkillTag::Control};
constexpr SkillDefinition kJoltspasmDefinition{
    SkillId::Joltspasm,
    "Joltspasm",
    std::span<const SkillTag>{kJoltspasmTags.data(), kJoltspasmTags.size()},
    5,
    SkillExecutionKind::DecalStrike,
    fl::widgets::effects::DecalAnimationKind::Shock,
};

} // namespace

const SkillDefinition &joltspasm_skill_definition() noexcept {
  return kJoltspasmDefinition;
}

} // namespace fl::skills
