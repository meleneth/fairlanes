#include <array>

#include "fl/skills/skill_definition.hpp"

namespace fl::skills {
namespace {

constexpr std::array kFlameWaveTags{SkillTag::Fire, SkillTag::Area};
constexpr SkillDefinition kFlameWaveDefinition{
    SkillId::FlameWave,
    "Flame Wave",
    std::span<const SkillTag>{kFlameWaveTags.data(), kFlameWaveTags.size()},
    2,
    0,
    SkillExecutionKind::FlameWave,
    fl::widgets::effects::DecalAnimationKind::FlameWave,
};

} // namespace

const SkillDefinition &flame_wave_skill_definition() noexcept {
  return kFlameWaveDefinition;
}

} // namespace fl::skills
