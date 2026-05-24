#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kGravitySighTags{SkillTag::Gravity, SkillTag::Control,
                                      SkillTag::Area};
constexpr SkillDefinition kGravitySighDefinition{
    SkillId::GravitySigh,
    "Gravity Sigh",
    std::span<const SkillTag>{kGravitySighTags.data(),
                              kGravitySighTags.size()},
    5,
    SkillExecutionKind::DecalStrike,
    fl::widgets::effects::DecalAnimationKind::VoidRipple,
};

} // namespace

const SkillDefinition &gravity_sigh_skill_definition() noexcept {
  return kGravitySighDefinition;
}

} // namespace fl::skills
