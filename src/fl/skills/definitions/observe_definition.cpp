#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kObserveTags{SkillTag::Observe, SkillTag::Utility};
constexpr SkillDefinition kObserveDefinition{
    SkillId::Observe,
    "Observe",
    std::span<const SkillTag>{kObserveTags.data(), kObserveTags.size()},
    0,
    SkillExecutionKind::Observe,
    std::nullopt,
};

} // namespace

const SkillDefinition &observe_skill_definition() noexcept {
  return kObserveDefinition;
}

} // namespace fl::skills
