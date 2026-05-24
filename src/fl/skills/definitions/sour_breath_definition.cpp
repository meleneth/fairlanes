#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kSourBreathTags{SkillTag::Acid, SkillTag::Disease,
                                     SkillTag::Area};
constexpr SkillDefinition kSourBreathDefinition{
    SkillId::SourBreath,
    "Sour Breath",
    std::span<const SkillTag>{kSourBreathTags.data(), kSourBreathTags.size()},
    5,
    SkillExecutionKind::DecalStrike,
    fl::widgets::effects::DecalAnimationKind::PoisonCloud,
};

} // namespace

const SkillDefinition &sour_breath_skill_definition() noexcept {
  return kSourBreathDefinition;
}

} // namespace fl::skills
