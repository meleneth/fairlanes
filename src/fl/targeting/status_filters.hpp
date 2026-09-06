#pragma once

#include <algorithm>
#include <array>
#include <ranges>

#include <entt/entt.hpp>

#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/poison.hpp"

namespace fl::targeting {

// Shared vocabulary for skill targeting and monster decision conditions.
enum class TargetStatus {
  None,
  Shield,
  Haste,
  Burn,
  Blind,
  Silence,
  Slow,
  Stun,
  Poison,
  DireBleed,
  Freeze
};

inline bool has_status(const entt::registry &reg, entt::entity entity,
                       TargetStatus status) {
  using namespace fl::ecs::components;
  if (!reg.valid(entity))
    return false;
  switch (status) {
  case TargetStatus::None:
    return false;
  case TargetStatus::Poison:
    return reg.all_of<Poison>(entity);
  case TargetStatus::DireBleed:
    return reg.all_of<DireBleed>(entity);
  case TargetStatus::Freeze:
    return reg.all_of<Freeze>(entity);
  default:
    break;
  }
  CombatStatusKind kind;
  switch (status) {
  case TargetStatus::Shield:
    kind = CombatStatusKind::Shield;
    break;
  case TargetStatus::Haste:
    kind = CombatStatusKind::Haste;
    break;
  case TargetStatus::Burn:
    kind = CombatStatusKind::Burn;
    break;
  case TargetStatus::Blind:
    kind = CombatStatusKind::Blind;
    break;
  case TargetStatus::Silence:
    kind = CombatStatusKind::Silence;
    break;
  case TargetStatus::Slow:
    kind = CombatStatusKind::Slow;
    break;
  case TargetStatus::Stun:
    kind = CombatStatusKind::Stun;
    break;
  default:
    return false;
  }
  const auto *statuses = reg.try_get<CombatStatuses>(entity);
  return statuses &&
         std::ranges::any_of(statuses->effects, [kind](const auto &effect) {
           return effect.kind == kind;
         });
}

// Adaptors own status IDs by value and borrow only the registry, not a context.
inline auto HasStatus(const entt::registry &reg, TargetStatus status) {
  return std::views::filter([&reg, status](entt::entity entity) {
    return has_status(reg, entity, status);
  });
}

inline auto LacksStatus(const entt::registry &reg, TargetStatus status) {
  return std::views::filter([&reg, status](entt::entity entity) {
    return reg.valid(entity) && !has_status(reg, entity, status);
  });
}

template <typename... Statuses>
auto HasAnyStatus(const entt::registry &reg, TargetStatus first,
                  Statuses... rest) {
  const std::array statuses{first, rest...};
  return std::views::filter([&reg, statuses](entt::entity entity) {
    return std::ranges::any_of(statuses, [&](TargetStatus status) {
      return has_status(reg, entity, status);
    });
  });
}

template <typename... Statuses>
auto HasAllStatuses(const entt::registry &reg, TargetStatus first,
                    Statuses... rest) {
  const std::array statuses{first, rest...};
  return std::views::filter([&reg, statuses](entt::entity entity) {
    return reg.valid(entity) &&
           std::ranges::all_of(statuses, [&](TargetStatus status) {
             return has_status(reg, entity, status);
           });
  });
}

inline auto ExcludeSelf(entt::entity actor) {
  return std::views::filter(
      [actor](entt::entity entity) { return entity != actor; });
}

} // namespace fl::targeting
