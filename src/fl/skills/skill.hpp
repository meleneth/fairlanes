#pragma once

#include <array>
#include <span>
#include <string_view>

namespace fl::skills {

enum class SkillId {
  Observe,
  Flee,
  Thump,
  Eviscerate,
  Poison,
  ColdSnap,
  FlameStrike,
  FlameWave,
  Joltspasm,
  RocksFall,
  SourBreath,
  Mercyburst,
  BloodBloom,
  IceSplitter,
  GravitySigh,
  Bump,
  Squish,
  Smack,
};

inline constexpr std::array<SkillId, 17> kRandomCombatSkills{
  SkillId::Thump,       SkillId::Eviscerate, SkillId::Poison,
  SkillId::ColdSnap,    SkillId::FlameStrike,
  SkillId::FlameWave,   SkillId::Bump,
  SkillId::Squish,      SkillId::Smack,
  SkillId::Joltspasm,   SkillId::RocksFall,
  SkillId::SourBreath,  SkillId::Mercyburst,
  SkillId::BloodBloom,  SkillId::IceSplitter,
  SkillId::GravitySigh, SkillId::Flee};

enum class SkillTag {
  Physical,
  Blunt,
  Piercing,
  Slashing,
  Bleed,
  Poison,
  Disease,
  Acid,
  Fire,
  Cold,
  Lightning,
  Earth,
  Gravity,
  Sonic,
  Healing,
  Holy,
  Control,
  Area,
  Projectile,
  Melee,
  Observe,
  Utility,
  Escape,
};

enum class SkillExecutionKind {
  ThumpLike,
  Eviscerate,
  Poison,
  ColdSnap,
  FlameStrike,
  FlameWave,
  DecalStrike,
  Flee,
  Observe,
};

struct SkillDefinition;

const SkillDefinition &definition(SkillId skill) noexcept;

std::string_view name(SkillId skill) noexcept;
int learn_chance_percent(SkillId skill) noexcept;
std::span<const SkillTag> tags(SkillId skill) noexcept;
bool has_tag(SkillId skill, SkillTag tag) noexcept;

} // namespace fl::skills
