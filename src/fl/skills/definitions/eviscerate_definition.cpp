#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kEviscerateTags{SkillTag::Physical, SkillTag::Slashing,
                                     SkillTag::Bleed, SkillTag::Melee};
constexpr SkillDefinition kEviscerateDefinition{
    SkillId::Eviscerate,
    "Eviscerate",
    std::span<const SkillTag>{kEviscerateTags.data(), kEviscerateTags.size()},
    2,
    0,
    SkillExecutionKind::Eviscerate,
    std::nullopt,
};

} // namespace

const SkillDefinition &eviscerate_skill_definition() noexcept {
  return kEviscerateDefinition;
}

} // namespace fl::skills
