#pragma once

#include <array>
#include <string_view>

namespace fl::skills {

enum class SkillId {
  Observe,
  Thump,
  Eviscerate,
  Bump,
  Squish,
  Smack,
};

inline constexpr std::array<SkillId, 5> kRandomCombatSkills{
    SkillId::Thump, SkillId::Eviscerate, SkillId::Bump, SkillId::Squish,
    SkillId::Smack};

constexpr std::string_view name(SkillId skill) noexcept {
  switch (skill) {
  case SkillId::Observe:
    return "Observe";
  case SkillId::Thump:
    return "Thump";
  case SkillId::Eviscerate:
    return "Eviscerate";
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
    return 2;
  case SkillId::Observe:
    return 0;
  }

  return 0;
}

} // namespace fl::skills
