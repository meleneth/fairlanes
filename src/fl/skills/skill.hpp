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
  Bump,
  Squish,
  Smack,
};

inline constexpr std::array<SkillId, 9> kRandomCombatSkills{
    SkillId::Thump,    SkillId::Eviscerate,  SkillId::Poison,
    SkillId::ColdSnap, SkillId::FlameStrike, SkillId::FlameWave,
    SkillId::Bump,     SkillId::Squish,      SkillId::Smack};

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
    return 5;
  case SkillId::Observe:
    return 0;
  }

  return 0;
}

} // namespace fl::skills
