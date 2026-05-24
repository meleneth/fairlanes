#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kFleeTags{SkillTag::Escape, SkillTag::Utility};
constexpr SkillDefinition kFleeDefinition{
    SkillId::Flee,
    "Flee",
    std::span<const SkillTag>{kFleeTags.data(), kFleeTags.size()},
    0,
    65,
    SkillExecutionKind::Flee,
    std::nullopt,
};

} // namespace

const SkillDefinition &flee_skill_definition() noexcept {
  return kFleeDefinition;
}

} // namespace fl::skills
