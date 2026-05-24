#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kBloodBloomTags{SkillTag::Bleed, SkillTag::Healing,
                                     SkillTag::Area};
constexpr SkillDefinition kBloodBloomDefinition{
    SkillId::BloodBloom,
    "Blood Bloom",
    std::span<const SkillTag>{kBloodBloomTags.data(), kBloodBloomTags.size()},
    5,
    0,
    SkillExecutionKind::DecalStrike,
    fl::widgets::effects::DecalAnimationKind::BloodBloom,
};

} // namespace

const SkillDefinition &blood_bloom_skill_definition() noexcept {
  return kBloodBloomDefinition;
}

} // namespace fl::skills
