#pragma once

#include <array>
#include <string_view>

namespace fl::skills {

enum class SkillId {
  Observe,
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

inline constexpr std::array<SkillId, 16> kRandomCombatSkills{
    SkillId::Thump,      SkillId::Eviscerate,  SkillId::Poison,
    SkillId::ColdSnap,   SkillId::FlameStrike, SkillId::FlameWave,
    SkillId::Bump,       SkillId::Squish,      SkillId::Smack,
    SkillId::Joltspasm,  SkillId::RocksFall,   SkillId::SourBreath,
    SkillId::Mercyburst, SkillId::BloodBloom,  SkillId::IceSplitter,
    SkillId::GravitySigh};

constexpr std::string_view name(SkillId skill) noexcept {
  switch (skill) {
  case SkillId::Observe:
    return "Observe";
  case SkillId::Thump:
    return "Thump";
  case SkillId::Eviscerate:
    return "Eviscerate";
  case SkillId::Poison:
    return "Poison";
  case SkillId::ColdSnap:
    return "Cold Snap";
  case SkillId::FlameStrike:
    return "Flame Strike";
  case SkillId::FlameWave:
    return "Flame Wave";
  case SkillId::Joltspasm:
    return "Joltspasm";
  case SkillId::RocksFall:
    return "Rocks Fall";
  case SkillId::SourBreath:
    return "Sour Breath";
  case SkillId::Mercyburst:
    return "Mercyburst";
  case SkillId::BloodBloom:
    return "Blood Bloom";
  case SkillId::IceSplitter:
    return "Ice Splitter";
  case SkillId::GravitySigh:
    return "Gravity Sigh";
  case SkillId::Bump:
    return "Bump";
  case SkillId::Squish:
    return "Squish";
  case SkillId::Smack:
    return "Smack";
  }

  return "Unknown";
}

constexpr int learn_chance_percent(SkillId skill) noexcept {
  switch (skill) {
  case SkillId::Thump:
  case SkillId::Bump:
  case SkillId::Squish:
  case SkillId::Smack:
    return 20;
  case SkillId::Eviscerate:
  case SkillId::FlameWave:
    return 2;
  case SkillId::Poison:
  case SkillId::ColdSnap:
  case SkillId::FlameStrike:
  case SkillId::Joltspasm:
  case SkillId::RocksFall:
  case SkillId::SourBreath:
  case SkillId::Mercyburst:
  case SkillId::BloodBloom:
  case SkillId::IceSplitter:
  case SkillId::GravitySigh:
    return 5;
  case SkillId::Observe:
    return 0;
  }

  return 0;
}

} // namespace fl::skills
