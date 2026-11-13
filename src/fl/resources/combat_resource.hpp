#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace fl::resources {

enum class CombatResource { Adrenaline, Reflexes, Count };
enum class ResourceChangeReason {
  SkillCost,
  SkillGeneration,
  SkillRestoration,
  SkillTargetModification,
  Potion
};

inline constexpr std::array kCombatResources{CombatResource::Adrenaline,
                                              CombatResource::Reflexes};

inline constexpr std::size_t index(CombatResource resource) noexcept {
  return static_cast<std::size_t>(resource);
}

inline constexpr std::string_view stable_id(CombatResource resource) noexcept {
  switch (resource) {
  case CombatResource::Adrenaline: return "adrenaline";
  case CombatResource::Reflexes: return "reflexes";
  case CombatResource::Count: break;
  }
  return "unknown";
}

inline constexpr std::string_view display_name(CombatResource resource) noexcept {
  switch (resource) {
  case CombatResource::Adrenaline: return "Adrenaline";
  case CombatResource::Reflexes: return "Reflexes";
  case CombatResource::Count: break;
  }
  return "Unknown";
}

} // namespace fl::resources
