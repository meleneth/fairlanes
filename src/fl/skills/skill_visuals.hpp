#pragma once

#include <optional>
#include <string_view>

#include "fl/skills/skill.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/widgets/effects/decal.hpp"

namespace fl::skills {

enum class SkillVisualArchetype {
  Impact,
  Slash,
  Bite,
  Projectile,
  Sweep,
  Burst,
  Beam,
  Heal,
  Cleanse,
  Glitch,
  Aura,
  Field,
  Observe,
};

struct SkillVisualSpec {
  SkillVisualArchetype archetype{SkillVisualArchetype::Impact};
  std::optional<fl::widgets::effects::DecalAnimationKind> custom_decal{};
};

[[nodiscard]] constexpr std::string_view
name(SkillVisualArchetype archetype) noexcept {
  switch (archetype) {
  case SkillVisualArchetype::Impact:
    return "Impact";
  case SkillVisualArchetype::Slash:
    return "Slash";
  case SkillVisualArchetype::Bite:
    return "Bite";
  case SkillVisualArchetype::Projectile:
    return "Projectile";
  case SkillVisualArchetype::Sweep:
    return "Sweep";
  case SkillVisualArchetype::Burst:
    return "Burst";
  case SkillVisualArchetype::Beam:
    return "Beam";
  case SkillVisualArchetype::Heal:
    return "Heal";
  case SkillVisualArchetype::Cleanse:
    return "Cleanse";
  case SkillVisualArchetype::Glitch:
    return "Glitch";
  case SkillVisualArchetype::Aura:
    return "Aura";
  case SkillVisualArchetype::Field:
    return "Field";
  case SkillVisualArchetype::Observe:
    return "Observe";
  }

  return "Unknown";
}

[[nodiscard]] inline std::optional<fl::widgets::effects::DecalAnimationKind>
decal_animation_for(SkillVisualArchetype archetype) noexcept {
  using fl::widgets::effects::DecalAnimationKind;

  switch (archetype) {
  case SkillVisualArchetype::Impact:
    return DecalAnimationKind::Impact;
  case SkillVisualArchetype::Slash:
    return DecalAnimationKind::Slash;
  case SkillVisualArchetype::Bite:
    return DecalAnimationKind::Bite;
  case SkillVisualArchetype::Projectile:
    return DecalAnimationKind::Projectile;
  case SkillVisualArchetype::Sweep:
    return DecalAnimationKind::Sweep;
  case SkillVisualArchetype::Burst:
    return DecalAnimationKind::Burst;
  case SkillVisualArchetype::Beam:
    return DecalAnimationKind::Beam;
  case SkillVisualArchetype::Heal:
    return DecalAnimationKind::Heal;
  case SkillVisualArchetype::Cleanse:
    return DecalAnimationKind::Cleanse;
  case SkillVisualArchetype::Glitch:
    return DecalAnimationKind::Glitch;
  case SkillVisualArchetype::Aura:
    return DecalAnimationKind::Aura;
  case SkillVisualArchetype::Field:
    return DecalAnimationKind::Field;
  case SkillVisualArchetype::Observe:
    return DecalAnimationKind::Observe;
  }

  return std::nullopt;
}

[[nodiscard]] inline SkillVisualArchetype
visual_archetype_for(SkillKey skill) noexcept {
  switch (skill.base) {
  case SkillId::Eviscerate:
  case SkillId::RakeLine:
  case SkillId::Claw:
    return SkillVisualArchetype::Slash;
  case SkillId::Bite:
    return SkillVisualArchetype::Bite;
  case SkillId::PebbleSpit:
  case SkillId::SnapShot:
  case SkillId::BurstFire:
  case SkillId::SuppressingFire:
  case SkillId::GrenadeLob:
  case SkillId::VenomNeedle:
    return SkillVisualArchetype::Projectile;
  case SkillId::ThumpAround:
  case SkillId::SmackdownSweep:
  case SkillId::FrostFan:
  case SkillId::LaserSweep:
    return SkillVisualArchetype::Sweep;
  case SkillId::Poison:
  case SkillId::RocksFall:
  case SkillId::FlameStrike:
  case SkillId::FlameWave:
  case SkillId::SourBreath:
  case SkillId::BloodBloom:
  case SkillId::IceSplitter:
  case SkillId::RotBloom:
  case SkillId::CaveIn:
  case SkillId::Squish:
    return SkillVisualArchetype::Burst;
  case SkillId::LaserStitch:
  case SkillId::PlasmaArc:
  case SkillId::Starblaze:
    return SkillVisualArchetype::Beam;
  case SkillId::Mercyburst:
  case SkillId::Mercywave:
  case SkillId::FieldDressing:
  case SkillId::RebootPulse:
    return SkillVisualArchetype::Heal;
  case SkillId::Clearbell:
  case SkillId::ChecksumWard:
    return SkillVisualArchetype::Cleanse;
  case SkillId::Joltspasm:
  case SkillId::ForkedJolt:
  case SkillId::StaticField:
  case SkillId::PacketStorm:
  case SkillId::SignalJam:
  case SkillId::BlueScreen:
  case SkillId::NullPointer:
    return SkillVisualArchetype::Glitch;
  case SkillId::ShellGuard:
  case SkillId::ArmorPlate:
  case SkillId::CinderVeil:
  case SkillId::RimeArmor:
  case SkillId::BattleFocus:
  case SkillId::PackHowl:
  case SkillId::Choirguard:
  case SkillId::SignalFlare:
  case SkillId::ClockUp:
  case SkillId::Overcharge:
  case SkillId::Burrow:
    return SkillVisualArchetype::Aura;
  case SkillId::SmokeScreen:
  case SkillId::Whiteout:
  case SkillId::MiasmaCloud:
  case SkillId::GnatCloud:
  case SkillId::WebSnare:
  case SkillId::WeightOfTuesday:
  case SkillId::HushHex:
  case SkillId::GravitySigh:
  case SkillId::EventHorizon:
    return SkillVisualArchetype::Field;
  case SkillId::Observe:
    return SkillVisualArchetype::Observe;
  case SkillId::Flee:
  case SkillId::Thump:
  case SkillId::ColdSnap:
  case SkillId::Bump:
  case SkillId::Smack:
  case SkillId::KindleWound:
  case SkillId::RootLeech:
  case SkillId::BumperRush:
    return SkillVisualArchetype::Impact;
  }

  return SkillVisualArchetype::Impact;
}

[[nodiscard]] inline SkillVisualSpec visual_spec_for(SkillKey skill) noexcept {
  if (!has_definition(skill)) {
    return {};
  }

  const auto &skill_definition = definition(skill);
  return SkillVisualSpec{visual_archetype_for(skill),
                         skill_definition.decal_animation};
}

[[nodiscard]] inline std::optional<fl::widgets::effects::DecalAnimationKind>
decal_animation_for(SkillKey skill) noexcept {
  const auto spec = visual_spec_for(skill);
  if (spec.custom_decal.has_value()) {
    return spec.custom_decal;
  }
  return decal_animation_for(spec.archetype);
}

} // namespace fl::skills
