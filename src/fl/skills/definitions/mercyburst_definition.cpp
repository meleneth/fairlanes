#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kMercyburstTags{SkillTag::Healing, SkillTag::Holy,
                                     SkillTag::Area};
constexpr SkillDefinition kMercyburstDefinition{
    SkillId::Mercyburst,
    "Mercyburst",
    std::span<const SkillTag>{kMercyburstTags.data(), kMercyburstTags.size()},
    5,
    SkillExecutionKind::DecalStrike,
    fl::widgets::effects::DecalAnimationKind::HolyNova,
};

} // namespace

const SkillDefinition &mercyburst_skill_definition() noexcept {
  return kMercyburstDefinition;
}

} // namespace fl::skills
