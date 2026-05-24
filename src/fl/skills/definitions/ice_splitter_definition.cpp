#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kIceSplitterTags{SkillTag::Cold, SkillTag::Piercing,
                                      SkillTag::Projectile};
constexpr SkillDefinition kIceSplitterDefinition{
    SkillId::IceSplitter,
    "Ice Splitter",
    std::span<const SkillTag>{kIceSplitterTags.data(),
                              kIceSplitterTags.size()},
    5,
    SkillExecutionKind::DecalStrike,
    fl::widgets::effects::DecalAnimationKind::FrostCrack,
};

} // namespace

const SkillDefinition &ice_splitter_skill_definition() noexcept {
  return kIceSplitterDefinition;
}

} // namespace fl::skills
