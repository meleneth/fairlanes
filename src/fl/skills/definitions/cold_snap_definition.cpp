#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kColdSnapTags{SkillTag::Cold, SkillTag::Control};
constexpr SkillDefinition kColdSnapDefinition{
    SkillId::ColdSnap,
    "Cold Snap",
    std::span<const SkillTag>{kColdSnapTags.data(), kColdSnapTags.size()},
    5,
    0,
    SkillExecutionKind::ColdSnap,
    std::nullopt,
};

} // namespace

const SkillDefinition &cold_snap_skill_definition() noexcept {
  return kColdSnapDefinition;
}

} // namespace fl::skills
