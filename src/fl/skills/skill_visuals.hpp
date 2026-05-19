#pragma once

#include <optional>

#include "fl/skills/skill.hpp"
#include "fl/widgets/effects/decal.hpp"

namespace fl::skills {

[[nodiscard]] constexpr std::optional<fl::widgets::effects::DecalAnimationKind>
decal_animation_for(SkillId skill) noexcept {
  using fl::widgets::effects::DecalAnimationKind;

  switch (skill) {
  case SkillId::FlameStrike:
  case SkillId::FlameWave:
    return DecalAnimationKind::FlameWave;
  case SkillId::Joltspasm:
    return DecalAnimationKind::Shock;
  case SkillId::RocksFall:
    return DecalAnimationKind::RocksFall;
  case SkillId::SourBreath:
    return DecalAnimationKind::PoisonCloud;
  case SkillId::Mercyburst:
    return DecalAnimationKind::HolyNova;
  case SkillId::BloodBloom:
    return DecalAnimationKind::BloodBloom;
  case SkillId::IceSplitter:
    return DecalAnimationKind::FrostCrack;
  case SkillId::GravitySigh:
    return DecalAnimationKind::VoidRipple;
  case SkillId::Observe:
  case SkillId::Thump:
  case SkillId::Eviscerate:
  case SkillId::Poison:
  case SkillId::ColdSnap:
  case SkillId::Bump:
  case SkillId::Squish:
  case SkillId::Smack:
    return std::nullopt;
  }

  return std::nullopt;
}

} // namespace fl::skills
